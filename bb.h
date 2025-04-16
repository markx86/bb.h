#ifndef BB_H_
#define BB_H_

#if defined(__linux__)
#	define BB_PLATFORM_LINUX
#elif defined(_WIN32)
#	define BB_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define BB_PLATFORM_APPLE
#else
#	error "Your platform is not currently supported by BB"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(BB_PLATFORM_WINDOWS)
#	include <windows.h>
#endif

#ifndef BB_DEFAULT_CC
# if defined(__GNUC__)
#		define BB_DEFAULT_CC "gcc"
# elif defined(__clang__)
#		define BB_DEFAULT_CC "clang"
# elif defined(_MSC_VER)
#		define BB_DEFAULT_CC "cl.exe"
# else
#		if defined(BB_PLATFORM_LINUX) || defined(BB_PLATFORM_APPLE)
#			define BB_DEFAULT_CC "cc"
#		else
#			error 																									\
				"Could not find default C compiler for your platform. \
				Define it with BB_DEFAULT_CC"
#		endif
#	endif
#endif

#ifndef BB_DISABLE_COLORS
#	define BB_RESET "\033[0m"
#	define BB_INFO "\033[1;36m"
#	define BB_WARN "\033[1;33m"
#	define BB_ERROR "\033[1;31m"
#	define BB_CRIT "\033[1;41;97m"
# define BB_DISABLE_COLORS
#else
#	define BB_RESET
#	define BB_INFO
#	define BB_WARN
#	define BB_ERROR
#	define BB_CRIT
#	undef BB_DISABLE_COLORS
#	define BB_DISABLE_COLORS "-DBB_DISABLE_COLORS"
#endif

#define bb_info(msg, ...) \
	fprintf(stdout, BB_INFO "[INFO]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_warn(msg, ...) \
	fprintf(stderr, BB_WARN "[WARN]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_error(msg, ...) \
	fprintf(stderr, BB_ERROR "[ERRO]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_crit(msg, ...) 																									\
	do {																																			\
		fprintf(stderr, BB_CRIT "[CRIT]" BB_RESET " " msg "\n", ##__VA_ARGS__);	\
		exit(EXIT_FAILURE);																											\
	} while (0)
#define bb_assert(x) 										\
	do {							 										\
		if (!(x))				 										\
			bb_crit("Assertion failed: " #x); \
	} while (0)

#define BB_UNUSED(x) ((void)(x))

#ifndef BB_STRING_MIN_CAPACITY
#	define BB_STRING_MIN_CAPACITY 64
#endif

typedef struct {
	size_t capacity;
	size_t length;
	char* cstr;
} *bb_string_t;

typedef struct bb_list {
	struct bb_list* next;
	struct bb_list* prev;
	void* elem;
} bb_list_t;

typedef struct {
	int argc;
	int envc;
	bb_string_t argv;
	bb_string_t envp;
} *bb_cmd_t;

#if defined(BB_PLATFORM_LINUX) || defined(BB_PLATFORM_APPLE)
	typedef pid_t bb_proc_t;
#elif defined(BB_PLATFORM_WINDOWS)
	typedef HANDLE bb_proc_t;
#else
#	error "Unsupported platform"
#endif

bb_string_t bb_string_new(size_t initial_capacity);
#define bb_string_default() bb_string_new(0)
bb_string_t bb_string_from_cstr(const char* cstr);
void bb_string_concat(bb_string_t dst, const char* src);
void bb_string_append(bb_string_t dst, char c);
void bb_string_destroy(bb_string_t* str);

void bb_file_copy(const char* src_path, const char* dst_path);
void bb_file_write(const char* path, const void* buffer, size_t size);
void* bb_file_read(const char* path);
void bb_file_free(void** buffer);

char* bb_args_next(int* argc, char*** argv);

bb_cmd_t bb_cmd_new(void);
void _bb_cmd_append_args(bb_cmd_t cmd, ...);
#define bb_cmd_append_args(cmd, ...) \
	_bb_cmd_append_args(cmd, ##__VA_ARGS__, NULL)
void _bb_cmd_append_envs(bb_cmd_t cmd, ...);
#define bb_cmd_append_envs(cmd, ...) \
	_bb_cmd_append_envs(cmd, ##__VA_ARGS__, NULL)
int _bb_cmd_run(bb_cmd_t cmd, ...);
#define bb_cmd_run(cmd, ...) \
	_bb_cmd_run(cmd, ##__VA_ARGS__, NULL)
bb_proc_t _bb_cmd_run_async(bb_cmd_t cmd, ...);
#define bb_cmd_run_async(cmd, ...) \
	_bb_cmd_run_async(cmd, ##__VA_ARGS__, NULL)
int bb_cmd_wait(bb_proc_t proc);
void bb_cmd_destroy(bb_cmd_t* cmd);
bb_string_t bb_cmd_to_string(bb_cmd_t cmd);

static inline void* bb_malloc(size_t size) {
	void* buffer;
	bb_assert(size > 0);
	buffer = malloc(size);
	bb_assert(buffer != NULL);
	return buffer;
}

static inline void* bb_zalloc(size_t size) {
	void* buffer = bb_malloc(size);
	memset(buffer, 0, size);
	return buffer;
}

static inline void* bb_realloc(void* buffer, size_t size) {
	bb_assert(size > 0); // Do not allow free() by realloc()
	bb_assert(buffer != NULL); // Do not allow malloc() by realloc()
	buffer = realloc(buffer, size);
	bb_assert(buffer != NULL);
	return buffer;
}

static inline void _bb_free(void* ptr) {
	void** buffer = (void**)ptr;
	bb_assert(buffer != NULL);
	bb_assert(*buffer != NULL);
	free(*buffer);
	*buffer = NULL;
}
#if defined(__GNUC__) || defined(__clang__)
// NOTE: This wrapper generates a warning when the type of
// 			 `x` is not a double-pointer :)
// NOTE: Other note. I don't know if there's a better way to do this :P
#	define bb_free(x) 										 																		 	 \
		do { 																 																		 	 \
			bb_assert((typeof(*(x)))(x) == (void*)(x) && "Type must be a double pointer!");\
			_bb_free(x);											 																		 	 \
		} while (0)
#else
// FIXME: MSVC has typeof only in C23 mode, cringe as hell
#	define bb_free(x) _bb_free(x)
#endif

#ifdef BB_IMPLEMENTATION

#ifndef BB_SOURCE
#	define BB_SOURCE "bb.c"
#endif

#ifndef BB_REBUILD_ARGS
#	if defined(__GNUC__) || defined(__clang__)
#		define BB_REBUILD_ARGS \
			BB_DISABLE_COLORS "-o", "bb", "-ggdb", "-Wall", "-Werror", BB_SOURCE
#	elif defined(_MSC_VER)
#		define BB_REBUILD_ARGS \
			BB_DISABLE_COLORS "-out:bb", "-Wall", "-WX", BB_SOURCE
#	else
#		error "Unknown C compiler"
#	endif
#endif

#include <stdarg.h>
#include <errno.h>

#if defined(BB_PLATFORM_LINUX) || defined(BB_PLATFORM_APPLE)
#	include <sys/stat.h>
#	include <sys/wait.h>
#	include <unistd.h>
#endif

#define BB_UNIMPLEMENTED_STUB() \
	bb_crit("%s() unimplemented for this platform", __func__)

static inline unsigned int _bb_proc_id(bb_proc_t handle) {
#ifdef BB_PLATFORM_WINDOWS
	return (unsigned int)GetProcessId(handle);
#else
	return (unsigned int)handle;
#endif
}

static bb_string_t _bb_strerror(void) {
	bb_string_t error_str;
#ifdef BB_PLATFORM_WINDOWS
	LPSTR error;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_ARGUMENT_ARRAY |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &error, 0, NULL);
	error_str = bb_string_from_cstr(error);
	LocalFree(error);
#else
	error_str = bb_string_from_cstr(strerror(errno));
#endif
	return error_str;
}

#ifdef BB_PLATFORM_WINDOWS
static bb_string_t _bb_to_windows_path(const char* path) {
	bb_string_t str = bb_string_default();
	if (strlen(path) == 0)
		return str;
	if (path[0] == '/')
		bb_string_concat(str, "C:");
	bb_string_concat(str, path);
	for (char* ptr = str->cstr; *ptr != '\0'; ++ptr) {
		if (*ptr == '/')
			*ptr = '\\';
	}
	return str;
}
#endif

static time_t _bb_file_last_modification_time(const char* path) {
	bb_string_t error;
#ifdef BB_PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA file_attr_data;
	bb_string_t windows_path = _bb_to_windows_path(path);
	if (GetFileAttributesExA(
				windows_path->cstr,
				GetFileExInfoStandard,
				&file_attr_data)) {
		bb_string_destroy(&windows_path);
		ULARGE_INTEGER time_full;
		FILETIME* time = &file_attr_data.ftLastWriteTime;
		time_full.u.LowPart = time->dwLowDateTime;
		time_full.u.HighPart = time->dwHighDateTime;
		return time_full.QuadPart / 1e9;
	}
#else
	struct stat info;
	if (stat(path, &info) == 0)
		return info.st_mtime;
#endif
	error = _bb_strerror();
	bb_crit("Could not get access time for %s: %s", path, error->cstr);
}

static void _bb_rebuild_if_needed(char** argv) {
	bb_cmd_t cmd;
	time_t bin, src;
	bb_assert(argv != NULL);
	bin = _bb_file_last_modification_time(argv[0]);
	src = _bb_file_last_modification_time(BB_SOURCE);
	if (src < bin)
		return;
	bb_info("Rebuilding %s...", BB_SOURCE);
	cmd = bb_cmd_new();
	bb_cmd_append_args(cmd, BB_DEFAULT_CC);
	bb_cmd_append_args(cmd, BB_REBUILD_ARGS);
	if (bb_cmd_run(cmd) != 0)
		bb_crit("Could not rebuild %s", BB_SOURCE);
	bb_cmd_destroy(&cmd);
	cmd = bb_cmd_new();
	while (*argv != NULL)
		bb_cmd_append_args(cmd, *(argv++));
	exit(bb_cmd_run(cmd));
}

bb_string_t bb_string_new(size_t initial_capacity) {
	bb_string_t str = bb_malloc(sizeof(*str));
	str->capacity =
		initial_capacity < BB_STRING_MIN_CAPACITY ?
		BB_STRING_MIN_CAPACITY : initial_capacity;
	str->length = 0;
	str->cstr = bb_zalloc(str->capacity);
	return str;
}

bb_string_t bb_string_from_cstr(const char* cstr) {
	bb_string_t str;
	bb_assert(cstr != NULL);
	str = bb_string_new(strlen(cstr) + 1);
	bb_string_concat(str, cstr);
	return str;
}

void bb_string_concat(bb_string_t dst, const char* src) {
	size_t needed_capacity, src_length;
	bb_assert(dst != NULL);
	bb_assert(src != NULL);
	src_length = strlen(src);
	needed_capacity = dst->length + src_length + 1;
	if (needed_capacity > dst->capacity) {
		dst->capacity = needed_capacity;
		dst->cstr = bb_realloc(dst->cstr, dst->capacity);
	}
	strcpy(&dst->cstr[dst->length], src);
	dst->length += src_length;
}

void bb_string_append(bb_string_t dst, char c) {
	bb_assert(dst != NULL);
	dst->cstr[dst->length] = c;
	if (++dst->length + 1 > dst->capacity) {
		dst->capacity = dst->length + BB_STRING_MIN_CAPACITY;
		dst->cstr = bb_realloc(dst->cstr, dst->capacity);
	}
	dst->cstr[dst->length] = '\0';
}

void bb_string_destroy(bb_string_t* str) {
	bb_assert(str != NULL);
	bb_assert(*str != NULL);
	bb_free(&(*str)->cstr);
	bb_free(str);
}

void bb_file_copy(const char* src_path, const char* dst_path) {
	FILE *src, *dst;
	bb_string_t error;
	size_t bytes_read;
	char buffer[4096];

	bb_assert(src_path != NULL);
	bb_assert(dst_path != NULL);

	src = fopen(src_path, "r");
	if (src == NULL)
		goto fail;

	dst = fopen(dst_path, "w");
	if (dst == NULL)
		goto fail;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
		if (fwrite(buffer, 1, bytes_read, dst) < bytes_read)
			goto fail;
	}

	if (ferror(src))
		goto fail;

	fclose(src);
	fclose(dst);
	return;

fail:
	error = _bb_strerror();
	bb_crit(
		"Could not copy file %s to %s: %s",
		src_path, dst_path, error->cstr);
	// NOTE: Unreachable
}

void bb_file_write(const char* path, const void* buffer, size_t size) {
	FILE* file;
	bb_string_t error;

	bb_assert(path != NULL);
	bb_assert(buffer != NULL);
	bb_assert(size > 0);

	file = fopen(path, "w");
	if (file == NULL)
		goto fail;

	if (fwrite(buffer, size, 1, file) != 1)
		goto fail;

	fclose(file);
	return;

fail:
	error = _bb_strerror();
	bb_crit("Could not write file %s: %s", path, error->cstr);
}

void bb_file_free(void** buffer) {
	bb_free(buffer);
}

void* bb_file_read(const char* path) {
	FILE* file;
	void* buffer;
	size_t size;
	bb_string_t error;

	bb_assert(path != NULL);

	file = fopen(path, "r");
	if (file == NULL)
		goto fail;

	if (fseek(file, 0, SEEK_END) < 0)
		goto fail;

	size = ftell(file);
	if ((long)size < 0)
		goto fail;

	rewind(file);

	buffer = bb_malloc(size);

	if (fread(buffer, size, 1, file) != 1)
		goto fail;

	fclose(file);
	return buffer;

fail:
	error = _bb_strerror();
	bb_crit("Could not read file %s: %s", path, error->cstr);
}

char* bb_args_next(int* argc, char*** argv) {
	bb_assert(argc != NULL);
	bb_assert(argv != NULL);
	bb_assert(*argv != NULL);
	if (*argc <= 0)
		return NULL;
	return ((*argv)++)[(*argc)--];
}

bb_cmd_t bb_cmd_new(void) {
	bb_cmd_t cmd = bb_malloc(sizeof(*cmd));
	cmd->argc = cmd->envc = 0;
	cmd->argv = bb_string_default();
	cmd->envp = bb_string_default();
	return cmd;
}

static void _bb_cmd_append_strings(
		int* count,
		bb_string_t str,
		va_list ap) {
	const char* s;
	for (; (s = va_arg(ap, const char*)) != NULL; ++*count) {
		if (str->length > 0)
			bb_string_append(str, ' ');
		bb_string_concat(str, s);
	}
}

void _bb_cmd_append_args(bb_cmd_t cmd, ...) {
	va_list ap;
	va_start(ap, cmd);
	_bb_cmd_append_strings(&cmd->argc, cmd->argv, ap);
	va_end(ap);
}

void _bb_cmd_append_envs(bb_cmd_t cmd, ...) {
	va_list ap;
	va_start(ap, cmd);
	_bb_cmd_append_strings(&cmd->envc, cmd->envp, ap);
	va_end(ap);
}

int bb_cmd_wait(bb_proc_t proc) {
	bb_string_t error;
#ifdef BB_PLATFORM_WINDOWS
	DWORD exit_code;
	if (WaitForSingleObject(proc, INFINITE) == WAIT_FAILED ||
			!GetExitCodeProcess(proc, &exit_code))
		goto fail;
	return exit_code;
#else
	int wstatus;
	if (waitpid(proc, &wstatus, 0) < 0 ||
			!WIFEXITED(wstatus))
		goto fail;
	return WEXITSTATUS(wstatus);
#endif
fail:
	error = _bb_strerror();
	bb_warn(
		"Could not wait for child process %u: %s",
		_bb_proc_id(proc), error->cstr);
	bb_info("Assuming child process failed");
	bb_string_destroy(&error);
	return EXIT_FAILURE;
}

static bb_string_t _bb_string_from_format(const char* fmt, va_list ap) {
	bb_string_t out;
	va_list orig;
	char* buffer;
	size_t buffer_size = 32768;
	va_copy(orig, ap);
	buffer = bb_malloc(buffer_size);
	while (vsnprintf(buffer, buffer_size, fmt, ap) == buffer_size) {
		buffer_size <<= 1;
		buffer = bb_realloc(buffer, buffer_size);
		va_copy(ap, orig);
	}
	out = bb_string_from_cstr(buffer);
	bb_free(&buffer);
	return out;
}

static char** _bb_string_to_null_terminated_array(
    bb_string_t str,
    char sep) {
	char** array;
	size_t elements;
	char* occurrence = str->cstr;

	for (elements = 0; str->length > 0 && occurrence != NULL; ++elements)
		occurrence = strchr(occurrence + 1, sep);

	array = bb_malloc((elements + 1) * sizeof(*array));
	array[0] = str->cstr;
	for (size_t i = 1; i <= elements; ++i) {
		array[i] = strchr(array[i-1], sep);
		if (array[i] == NULL)
			break;
		*(array[i]++) = '\0';
	}

	return array;
}

static bb_proc_t _bb_cmd_execute(bb_cmd_t cmd, va_list ap) {
	bb_proc_t proc;
	bb_string_t cmdline, cmdenv, error;
	char **argv, **envp;

	cmdline = _bb_string_from_format(cmd->argv->cstr, ap);
	cmdenv = _bb_string_from_format(cmd->envp->cstr, ap);
	bb_info("Executing: %s", cmdline->cstr);
	if (cmd->envc > 0)
		bb_info("- with environment: %s", cmdenv->cstr);

	envp = _bb_string_to_null_terminated_array(cmdenv, ' ');
#ifdef BB_PLATFORM_WINDOWS
	PROCESS_INFORMATION proc_info = {0};
	STARTUPINFOA startup_info = {0};
	startup_info.cb = sizeof(startup_info);
	if (!CreateProcessA(
				NULL, cmdline->cstr, NULL, NULL,
				FALSE, NORMAL_PRIORITY_CLASS, envp,
				NULL, &startup_info, &proc_info))
		goto fail;
	proc = proc_info.hProcess;
#else
	argv = _bb_string_to_null_terminated_array(cmdline, ' ');
	proc = fork();
	if (proc < 0)
		goto fail;
	if (proc == 0) {
		for (size_t e = 0; e < cmd->envc; ++e)
			putenv(envp[e]);
		if (execvp(argv[0], argv) < 0)
			bb_crit("Could not execute command: %s", cmdline->cstr);
	}
	bb_free(&argv);
#endif
	bb_free(&envp);

	bb_string_destroy(&cmdline);
	bb_string_destroy(&cmdenv);

	bb_info("- as process: %u", _bb_proc_id(proc));

	return proc;

fail:
	error = _bb_strerror();
	bb_crit(
		"Could not run command: %s: %s",
		cmdline->cstr, error->cstr);
}

bb_proc_t _bb_cmd_run_async(bb_cmd_t cmd, ...) {
	bb_proc_t proc;
	va_list ap;
	va_start(ap, cmd);
	proc = _bb_cmd_execute(cmd, ap);
	va_end(ap);
	return proc;
}

int _bb_cmd_run(bb_cmd_t cmd, ...) {
	int exit_status;
	va_list ap;
	va_start(ap, cmd);
	exit_status = bb_cmd_wait(_bb_cmd_execute(cmd, ap));
	va_end(ap);
	return exit_status;
}

void bb_cmd_destroy(bb_cmd_t* cmd) {
	bb_assert(cmd != NULL);
	bb_assert(*cmd != NULL);
	bb_string_destroy(&(*cmd)->argv);
	bb_string_destroy(&(*cmd)->envp);
	bb_free(cmd);
}

extern int bb_main(int argc, char** argv, char** envp);

int main(int argc, char** argv, char** envp) {
	bb_assert(argc >= 1);
	_bb_rebuild_if_needed(argv);
	return bb_main(argc, argv, envp);
}

#endif

#endif
