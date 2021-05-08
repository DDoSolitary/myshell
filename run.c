#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "cmd.h"
#include "error.h"
#include "run.h"
#include "vector.h"

static bool fix_fds(command_t *cmd, int fd) {
	for (size_t i = 0; i < command_redir_size(cmd); i++) {
		redir_t *redir = command_redir_at(cmd, i);
		if (REDIR_IS_FILE(redir->type) && redir->from_fd == fd) {
			redir->from_fd = fcntl(redir->from_fd, F_DUPFD_CLOEXEC, 0);
			if (redir->from_fd < 0) {
				print_errno("fcntl(%d, F_DUPFD_CLOEXEC, 0)", redir->from_fd);
				return false;
			}
		}
	}
	return true;
}

static bool init_child_fds(command_t *cmd) {
	for (size_t i = 0; i < command_redir_size(cmd); i++) {
		redir_t *redir = command_redir_at(cmd, i);
		if (!fix_fds(cmd, redir->to_fd)) {
			return false;
		}
		if (redir->from_fd < 0) {
			close(redir->to_fd);
		} else if (dup2(redir->from_fd, redir->to_fd) < 0) {
			print_errno("dup2(%d, %d)", redir->from_fd, redir->to_fd);
			return false;
		}
	}
	return true;
}

static bool check_fd(command_t *cmd, size_t len, int fd, redir_type_t redir_type) {
	for (size_t i = len - 1; i < len; i--) {
		redir_t *redir = command_redir_at(cmd, i);
		if (redir->to_fd == fd) {
			if (redir_type == REDIR_FD_IN && !REDIR_IS_READABLE(redir->type)) {
				print_err("fd %d isn't open for reading", fd);
				return false;
			}
			if (redir_type == REDIR_FD_OUT && !REDIR_IS_WRITABLE(redir->type)) {
				print_err("fd %d isn't open for writing", fd);
				return false;
			}
			return true;
		}
	}
	print_err("fd %d isn't open", fd);
	return false;
}

static void close_fds(vec_t *fds) {
	for (size_t i = 0; i < vec_size(fds); i++) {
		int fd = *(int *)vec_at(fds, i);
		close(fd);
	}
	vec_free(fds);
}

static bool run_command(command_t *cmd, int fd0, int fd1, pid_t *pid) {
	vec_t closing_fds;
	vec_init(&closing_fds, sizeof(int));
	if (fd0 >= 0) {
		command_redir_at(cmd, 0)->from_fd = fd0;
		vec_push_back(&closing_fds, &fd0);
	}
	if (fd1 >= 0) {
		command_redir_at(cmd, 1)->from_fd = fd1;
		vec_push_back(&closing_fds, &fd1);
	}
	for (size_t i = 3; i < command_redir_size(cmd); i++) {
		redir_t *redir = command_redir_at(cmd, i);
		if (REDIR_IS_FD(redir->type)) {
			if (!strcmp(redir->from_path, "-")) {
				redir->from_fd = -1;
			} else {
				char *end;
				long fd = strtol(redir->from_path, &end, 10);
				if (*end || fd < 0 || fd > INT_MAX) {
					print_err("invalid fd %s", redir->from_path);
					close_fds(&closing_fds);
					return false;
				}
				redir->from_fd = (int)fd;
				if (!check_fd(cmd, i, redir->from_fd, redir->type)) {
					close_fds(&closing_fds);
					return false;
				}
			}
		} else {
			int flags = 0;
			switch (redir->type) {
			case REDIR_FILE_IN:
				flags = O_RDONLY;
				break;
			case REDIR_FILE_TRUNC:
				flags = O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case REDIR_FILE_APPEND:
				flags = O_WRONLY | O_CREAT | O_APPEND;
				break;
			case REDIR_FILE_IN_OUT:
				flags = O_RDWR | O_CREAT;
				break;
			default:
				abort();
			}
			flags |= O_CLOEXEC;
			int fd = open(redir->from_path, flags, 0666);
			if (fd < 0) {
				print_errno("open(%s, 0x%x, 0666)", redir->from_path, flags);
				close_fds(&closing_fds);
				return false;
			}
			redir->from_fd = fd;
			vec_push_back(&closing_fds, &fd);
		}
	}
	int ret = fork();
	if (ret < 0) {
		print_errno("fork");
		close_fds(&closing_fds);
		return false;
	}
	if (ret == 0) {
		if (!init_child_fds(cmd)) {
			exit(1);
		}
		char **args = command_arg_at(cmd, 0);
		execvp(args[0], args);
		print_errno("%s", args[0]);
		switch (errno) {
		case ENOENT:
			exit(127);
		case EACCES:
			exit(126);
		default:
			exit(1);
		}
	}
	*pid = (pid_t)ret;
	close_fds(&closing_fds);
	return true;
}

static int pipeline_cleanup(int pipe, vec_t *pid_list, bool show_error) {
	if (pipe >= 0) {
		close(pipe);
	}
	int ret = 1;
	for (size_t i = 0; i < vec_size(pid_list); i++) {
		pid_t pid = *(pid_t *)vec_at(pid_list, i);
		int status;
		if (waitpid(pid, &status, 0) < 0) {
			if (show_error) {
				print_errno("waitpid(%d, &status, 0)", pid);
			}
		} else if (i == vec_size(pid_list) - 1) {
			if (WIFSIGNALED(status)) {
				ret = 129;
				if (show_error) {
					print_err("process terminated by signal: %s", strsignal(WTERMSIG(status)));
				}
			} else if (WIFEXITED(status)) {
				ret = WEXITSTATUS(status);
				if (show_error && ret) {
					print_err("process exited with status %d", ret);
				}
			}
		}
	}
	vec_free(pid_list);
	return ret;
}

int run_pipeline(pipeline_t *pipeline) {
	vec_t pid_list;
	vec_init(&pid_list, sizeof(pid_t));
	int prev_pipe = -1;
	int pipes[2];
	for (size_t i = 0; i < pipeline_size(pipeline); i++) {
		if (i < pipeline_size(pipeline) - 1) {
			if (pipe2(pipes, O_CLOEXEC) < 0) {
				print_errno("pipe2");
				pipeline_cleanup(prev_pipe, &pid_list, false);
				return 1;
			}
		} else {
			pipes[0] = pipes[1] = -1;
		}
		command_t *cmd = pipeline_at(pipeline, i);
		pid_t pid;
		if (run_command(cmd, prev_pipe, pipes[1], &pid)) {
			vec_push_back(&pid_list, &pid);
		}
		prev_pipe = pipes[0];
	}
	return pipeline_cleanup(prev_pipe, &pid_list, true);
}
