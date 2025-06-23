#ifndef BB_H_
#define BB_H_

#if defined(__linux__)
# define BB_PLATFORM_LINUX
#elif defined(_WIN32)
# define BB_PLATFORM_WINDOWS
#elif defined(__APPLE__)
# define BB_PLATFORM_APPLE
#else
# error "Your platform is not currently supported by BB"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(BB_PLATFORM_WINDOWS)
# include <windows.h>
#endif

#ifndef BB_DEFAULT_CC
# if defined(__GNUC__)
#   define BB_DEFAULT_CC "gcc"
# elif defined(__clang__)
#   define BB_DEFAULT_CC "clang"
# elif defined(_MSC_VER)
#   define BB_DEFAULT_CC "cl.exe"
# else
#   if defined(BB_PLATFORM_LINUX) || defined(BB_PLATFORM_APPLE)
#     define BB_DEFAULT_CC "cc"
#   else
#     error "Could not find default C compiler for your platform. \
             Define it with BB_DEFAULT_CC."
#   endif
# endif
#endif

#ifndef BB_DISABLE_COLORS
# define BB_RESET "\033[0m"
# define BB_INFO "\033[1;36m"
# define BB_WARN "\033[1;33m"
# define BB_ERROR "\033[1;31m"
# define BB_CRIT "\033[1;41;97m"
# define BB_BOLD "\033[1m"
# define BB_DISABLE_COLORS
#else
# define BB_RESET
# define BB_INFO
# define BB_WARN
# define BB_ERROR
# define BB_CRIT
# define BB_BOLD
# undef BB_DISABLE_COLORS
# define BB_DISABLE_COLORS "-DBB_DISABLE_COLORS"
#endif

#define bb_info(msg, ...) \
  fprintf(stdout, BB_INFO "[INFO]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_warn(msg, ...) \
  fprintf(stderr, BB_WARN "[WARN]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_error(msg, ...) \
  fprintf(stderr, BB_ERROR "[ERRO]" BB_RESET " " msg "\n", ##__VA_ARGS__)
#define bb_crit(msg, ...)                                                   \
  do {                                                                      \
    fprintf(stderr, BB_CRIT "[CRIT]" BB_RESET " " msg "\n", ##__VA_ARGS__);  \
    exit(EXIT_FAILURE);                                                      \
  } while (0)
#define bb_assert(x)                     \
  do {                                   \
    if (!(x))                             \
      bb_crit("Assertion failed: " #x); \
  } while (0)

#define BB_UNUSED(x) ((void)(x))

#ifndef BB_STRING_MIN_CAPACITY
# define BB_STRING_MIN_CAPACITY 64
#endif

#ifndef BB_VECTOR_MIN_CAPACITY
# define BB_VECTOR_MIN_CAPACITY 16
#endif

#define BB_TRUE  1
#define BB_FALSE 0

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
# error "Unsupported platform"
#endif

#if defined(__GNUC__) || defined(__clang__)
// NOTE: This wrapper generates a warning when the type of
//        `x` is not a double-pointer :)
// NOTE: Other note. I don't know if there's a better way to do this :P
# define BB_ENSURE_DOUBLEPTR(x) \
    bb_assert((typeof(*(x)))(x) == (void*)(x) && \
              "Type must be a double pointer!");
#else
// FIXME: MSVC has typeof only in C23 mode, cringe as hell.
# define BB_ENSURE_DOUBLEPTR(x) bb_warn("Switch to a serious compiler")
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
int bb_file_was_modified(const char* path);

const char* bb_params_get_string(const char* long_name, char short_name,
                                 const char* help, const char* default_value);
long bb_params_get_int(const char* long_name, char short_name,
                       const char* help, const long* default_value);
double bb_params_get_float(const char* long_name, char short_name,
                           const char* help, const double* default_value);
int bb_params_get_switch(const char* long_name, char short_name,
                         const char* help, int default_value);

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

const void* _bb_vector_new(size_t item_size, size_t length);
#define bb_vector_new(T, L) _bb_vector_new(sizeof(T), L)
#define bb_vector_default(T) bb_vector_new(T, 0)
void _bb_vector_push(const void** vec, void* elem);
#define bb_vector_push(vec, elem)              \
  do {                                         \
    BB_ENSURE_DOUBLEPTR(&vec);                 \
    _bb_vector_push((const void**)&vec, elem); \
  } while (0)
long bb_vector_pop(const void* vec, void* elem);
size_t bb_vector_length(const void* vec);
size_t bb_vector_capacity(const void* vec);
void _bb_vector_destroy(const void** vec);
#define bb_vector_destroy(vec)              \
  do {                                      \
    BB_ENSURE_DOUBLEPTR(&vec);              \
    _bb_vector_destroy((const void**)&vec); \
  } while (0)

static inline char* bb_strdup(const char* s) {
  char* duped;
  bb_assert(s != NULL);
  duped = strdup(s);
  bb_assert(duped != NULL);
  return duped;
}

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
  bb_assert(size > 0);       // Do not allow free() by realloc().
  bb_assert(buffer != NULL); // Do not allow malloc() by realloc().
  buffer = realloc(buffer, size);
  bb_assert(buffer != NULL);
  return buffer;
}

static inline void _bb_free(void** buffer) {
  bb_assert(buffer != NULL);
  bb_assert(*buffer != NULL);
  free(*buffer);
  *buffer = NULL;
}

#define bb_free(x)           \
  do {                       \
    BB_ENSURE_DOUBLEPTR(x);  \
    _bb_free((void**)x);     \
  } while (0)

#ifdef BB_PLATFORM_WINDOWS
char* bb_path(const char* path);
#else
#define bb_path(x) bb_strdup(x)
#endif

int bb_main(void);

#ifdef BB_IMPLEMENTATION

#ifndef BB_SOURCE
# define BB_SOURCE "bb.c"
#endif

#ifndef BB_REBUILD_ARGS
# if defined(__GNUC__) || defined(__clang__)
#   define BB_REBUILD_ARGS \
      BB_DISABLE_COLORS "-o", "bb", "-ggdb", "-Wall", "-Werror", BB_SOURCE
# elif defined(_MSC_VER)
#   define BB_REBUILD_ARGS \
      BB_DISABLE_COLORS "-out:bb", "-Wall", "-WX", BB_SOURCE
# else
#   error "Could not determine rebuild arguments from compiler. \
           Define them with BB_REBUILD_ARGS."
# endif
#endif

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#if defined(BB_PLATFORM_LINUX) || defined(BB_PLATFORM_APPLE)
# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>
#endif

#define BB_UNIMPLEMENTED_STUB() \
  bb_crit("%s() unimplemented for this platform", __func__)

typedef struct {
  unsigned int capacity;
  unsigned int length;
  unsigned int item_size;
  unsigned int checksum;
} *_bb_vector_t;

static time_t _bb_ref_time;

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
  const DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_ARGUMENT_ARRAY
                      | FORMAT_MESSAGE_FROM_SYSTEM;
  FormatMessageA(dwFlags, NULL, GetLastError(),
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
char* bb_path(const char* path) {
  size_t path_len;
  char* str;
  bb_assert(path != NULL);
  path_len = strlen(path);
  if (path_len == 0)
    return path;
  str = bb_malloc(path_len + 1 + ((path[0] == '/') << 1));
  // If the UNIX path is absolute, make the Windows path
  // start with 'C:'.
  if (path[0] == '/') {
    str[0] = 'C';
    str[1] = ':';
  }
  strncat(str, path, path_len);
  // Replace / with \ (we have to do this because Windows is stupid).
  for (char* ptr = str; *ptr != '\0'; ++ptr) {
    if (*ptr == '/')
      *ptr = '\\';
  }
  return str;
}
#endif

static time_t _bb_file_last_modification_time(const char* path) {
  bb_string_t error;
  bb_assert(path != NULL);
#ifdef BB_PLATFORM_WINDOWS
  WIN32_FILE_ATTRIBUTE_DATA file_attr_data;
  ULARGE_INTEGER time_full;
  LPFILETIME time;
  char* windows_path = bb_path(path);
  if (GetFileAttributesExA(windows_path,
                           GetFileExInfoStandard,
                           &file_attr_data)) {
    bb_free(&windows_path);
    time = &file_attr_data.ftLastWriteTime;
    time_full.u.LowPart = time->dwLowDateTime;
    time_full.u.HighPart = time->dwHighDateTime;
    return time_full.QuadPart * 1e2;
  }
#else
  struct stat info;
  // NOTE: We do not use bb_path(..) here since this code will be run
  // only on *NIX systems, therefore the path will already be in the
  // correct form.
  if (stat(path, &info) == 0)
    return info.st_mtim.tv_sec * 1e9 + info.st_mtim.tv_nsec;
#endif
  error = _bb_strerror();
  bb_crit("Could not get access time for %s: %s", path, error->cstr);
}

static void _bb_rebuild_if_needed(char** argv) {
  bb_cmd_t cmd;
  time_t src, bin;
  char ref_time[32];

  bb_assert(argv != NULL);

  bin = _bb_file_last_modification_time(argv[0]);
  src = _bb_file_last_modification_time(BB_SOURCE);
  // Save the bin time as a reference.
  _bb_ref_time = bin;
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

  // Pass along the current time reference, unless the user has already
  // specified it in the environment.
  if (getenv("BB_REF_TIME") == NULL) {
    snprintf(ref_time, sizeof(ref_time), "BB_REF_TIME=%lu", _bb_ref_time);
    bb_cmd_append_envs(cmd, ref_time);
  }

  exit(bb_cmd_run(cmd));
}

// NOTE: This function updates the modification time of the bb executable.
// (sussy function name o_o)
static void _bb_touch_self(const char* self) {
  bb_string_t error;
#ifdef BB_PLATFORM_WINDOWS
  BB_UNIMPLEMENTED_STUB();
#else
  int rc, fd = open(self, O_RDONLY);
  if (fd < 0)
    goto fail;
  rc = futimens(fd, NULL);
  close(fd);
  if (rc < 0)
    goto fail;
#endif
  return;
fail:
  error = _bb_strerror();
  bb_error("Could not touch self: %s", error->cstr);
  bb_string_destroy(&error);
}

bb_string_t bb_string_new(size_t initial_capacity) {
  bb_string_t str = bb_malloc(sizeof(*str));
  str->capacity = initial_capacity < BB_STRING_MIN_CAPACITY ?
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
  char *src_path2, *dst_path2;
  char buffer[4096];

  bb_assert(src_path != NULL);
  bb_assert(dst_path != NULL);
  // Normalize paths.
  src_path2 = bb_path(src_path);
  dst_path2 = bb_path(dst_path);

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

  fclose(dst);
  fclose(src);
  bb_free(&dst_path2);
  bb_free(&src_path2);
  return;

fail:
  error = _bb_strerror();
  bb_crit("Could not copy file %s to %s: %s",
          src_path, dst_path, error->cstr);
  // NOTE: Unreachable.
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

int bb_file_was_modified(const char* path) {
  time_t mtime = _bb_file_last_modification_time(path);
  return _bb_ref_time < mtime;
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

static void _bb_cmd_append_strings(int* count, bb_string_t str, va_list ap) {
  const char* s;
  bb_assert(count != NULL);
  bb_assert(str != NULL);
  for (; (s = va_arg(ap, const char*)) != NULL; ++*count) {
    if (str->length > 0)
      bb_string_append(str, ' ');
    bb_string_concat(str, s);
  }
}

void _bb_cmd_append_args(bb_cmd_t cmd, ...) {
  va_list ap;
  bb_assert(cmd != NULL);
  va_start(ap, cmd);
  _bb_cmd_append_strings(&cmd->argc, cmd->argv, ap);
  va_end(ap);
}

void _bb_cmd_append_envs(bb_cmd_t cmd, ...) {
  va_list ap;
  bb_assert(cmd != NULL);
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
  bb_warn("Could not wait for child process %u: %s",
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

  bb_assert(fmt != NULL);

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

static char** _bb_string_to_null_terminated_array(bb_string_t str, char sep) {
  char** array;
  size_t elements;
  char* occurrence = str->cstr;

  bb_assert(str != NULL);

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

  bb_assert(cmd != NULL);

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
  if (!CreateProcessA(NULL, cmdline->cstr, NULL, NULL,
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
  bb_crit("Could not run command: %s: %s",
          cmdline->cstr, error->cstr);
}

bb_proc_t _bb_cmd_run_async(bb_cmd_t cmd, ...) {
  bb_proc_t proc;
  va_list ap;
  bb_assert(cmd != NULL);
  va_start(ap, cmd);
  proc = _bb_cmd_execute(cmd, ap);
  va_end(ap);
  return proc;
}

int _bb_cmd_run(bb_cmd_t cmd, ...) {
  int exit_status;
  va_list ap;
  bb_assert(cmd != NULL);
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

static inline unsigned int _bb_vector_compute_checksum(_bb_vector_t vec) {
  return -(vec->capacity + vec->length + vec->item_size);
}

static inline void _bb_vector_validate_checksum(_bb_vector_t vec) {
  bb_assert((vec->item_size + vec->length +
             vec->capacity + vec->checksum) == 0);
}

static inline _bb_vector_t _bb_vector_get(const void* ptr) {
  _bb_vector_t vec;
  bb_assert(ptr != NULL);
  vec = ((_bb_vector_t)ptr) - 1;
  _bb_vector_validate_checksum(vec);
  return vec;
}

const void* _bb_vector_new(size_t item_size, size_t capacity) {
  _bb_vector_t vec;

  bb_assert(item_size > 0);

  if (capacity < BB_VECTOR_MIN_CAPACITY)
    capacity = BB_VECTOR_MIN_CAPACITY;

  vec = bb_malloc(capacity * item_size + sizeof(*vec));
  vec->item_size = item_size;
  vec->capacity = capacity;
  vec->length = 0;
  vec->checksum = _bb_vector_compute_checksum(vec);

  // It hurts me to cast this to a const void*, but I really
  // don't want people writing in to vector memory willy-nilly,
  // so this should indicate to people that they should use `push` and `pop`
  // instead, and only use direct access for accessing data.
  return vec + 1;
}

void _bb_vector_push(const void** vec_ptr, void* elem) {
  _bb_vector_t vec;
  void* raw_ptr;

  bb_assert(vec_ptr != NULL);
  vec = _bb_vector_get(*vec_ptr);
  if (vec->length + 1 >= vec->capacity) {
    bb_assert((vec->capacity >> 31) == 0); // Ensure we don't overflow U32.
    vec->capacity <<= 1;
    vec = bb_realloc(vec, vec->capacity * vec->item_size + sizeof(*vec));
    *vec_ptr = vec;
  }

  raw_ptr = ((char*)*vec_ptr) + vec->item_size * vec->length++;
  memcpy(raw_ptr, elem, vec->item_size);

  vec->checksum = _bb_vector_compute_checksum(vec);
}

long bb_vector_pop(const void* vec_ptr, void* elem) {
  _bb_vector_t vec = _bb_vector_get(vec_ptr);
  unsigned int len;
  void* raw_ptr;

  if ((len = vec->length) == 0)
    goto out;

  raw_ptr = ((char*)vec_ptr) + vec->item_size * --vec->length;
  memcpy(elem, raw_ptr, vec->item_size);

  vec->checksum = _bb_vector_compute_checksum(vec);
out:
  return len - 1;
}

size_t bb_vector_length(const void* vec_ptr) {
  return _bb_vector_get(vec_ptr)->length;
}

size_t bb_vector_capacity(const void* vec_ptr) {
  return _bb_vector_get(vec_ptr)->capacity;
}

void _bb_vector_destroy(const void** vec_ptr) {
  _bb_vector_t vec;
  bb_assert(vec_ptr != NULL);
  vec = _bb_vector_get(*vec_ptr);
  *vec_ptr = NULL;
  bb_free(&vec);
}

typedef struct {
  int argc;
  const char* const* argv;
  const char* const* envp;
} *_bb_params_t;

static _bb_params_t params;

static const char* _bb_param_get_env_name(const char* long_name) {
  size_t name_len;
  char *env_name, *e;

  bb_assert(long_name != NULL);
  name_len = strlen(long_name) + 3;

  e = env_name = bb_malloc(name_len);
  *(e++) = 'B';
  *(e++) = 'B';
  *(e++) = '_';

  for (const char* n = long_name; *n; ++n)
    *(e++) = isalnum(*n) ? toupper(*n) : '_';

  return env_name;
}

static const char* _bb_params_find_by_name(const char* long_name,
                                           char short_name,
                                           int has_value) {
  size_t name_len;
  const char *arg, *env_name, *param_value;
  const char* const* argv;

  bb_assert(params != NULL);
  bb_assert(long_name != NULL);

  argv = params->argv;
  name_len = strlen(long_name);
  while ((arg = *(argv++)) != NULL) {
    if (*(arg++) != '-')
      continue; // Arg. does not start with '-', skip.
    if (*arg == short_name)
      break;    // Arg. matches the short name, we found our guy!
    if (*(arg++) != '-')
      continue; // Arg. does not start with '--', skip.
    if (!strncmp(long_name, arg, name_len))
      break;    // Arg. matches the long name, we found the param!
  }
  if (arg == NULL) {
    // Did not find parameter in argv, search in environment variables.
    env_name = _bb_param_get_env_name(long_name);
    param_value = getenv(env_name);
    bb_free(&env_name);
    // Always return value as is, if it's an env var.
    return param_value;
  }

  if (!has_value)
    return "";

  if ((param_value = strchr(arg, '=')) == NULL)
    return *argv == NULL ? "" : *argv;
  else
    return param_value + 1;
}

typedef enum {
  _BB_PARAM_STRING,
  _BB_PARAM_LONG,
  _BB_PARAM_DOUBLE,
  _BB_PARAM_SWITCH,
  _BB_PARAM_TYPE_MAX
} _bb_param_type_t;

static void _bb_param_print_help(const char* long_name, char short_name,
                                 _bb_param_type_t type, const char* help,
                                 const void* default_value) {
  char buffer[32] = "None";
  const char *type_str, *value_str = buffer;
  const char short_str[] = {
    ' ', '(', 'o', 'r', ' ', '-', short_name, ')', '\0'
  };

  switch (type) {
    case _BB_PARAM_STRING:
      if (default_value)
        value_str = default_value;
      type_str = "string";
      break;
    case _BB_PARAM_LONG:
      if (default_value != NULL)
        snprintf(buffer, sizeof(buffer),
                 "%ld", *(const long*)default_value);
      type_str = "integer";
      break;
    case _BB_PARAM_DOUBLE:
      if (default_value != NULL)
        snprintf(buffer, sizeof(buffer),
                 "%lf", *(const double*)default_value);
      type_str = "real number";
      break;
    case _BB_PARAM_SWITCH:
      bb_assert(default_value != NULL);
      snprintf(buffer, sizeof(buffer),
               "%s", (*(int*)default_value == BB_TRUE) ? "true" : "false");
      type_str = "switch";
      break;
    default:
      // NOTE: This branch always fails.
      bb_assert(type < _BB_PARAM_TYPE_MAX);
      break;
  }

  bb_info("Info for parameter " BB_BOLD "--%s%s" BB_RESET "\n"
          "       | Type: %s\n"
          "       | Default value: %s\n"
          "       | Help: %s\n",
          long_name, short_name ? short_str : "", type_str, value_str,
          help ? help : "No help provided.");
}

static inline void _bb_param_missing(const char* long_name, char short_name,
                                     _bb_param_type_t type, const char* help) {
  bb_error("Required parameter missing.");
  _bb_param_print_help(long_name, short_name, type, help, NULL);
  exit(EXIT_FAILURE);
}

static inline void _bb_param_invalid(const char* value,
                                     const char* long_name, char short_name,
                                     _bb_param_type_t type, const char* help,
                                     const void* default_value) {
  bb_error("Invalid value '%s' for parameter.", value);
  _bb_param_print_help(long_name, short_name, type, help, default_value);
  exit(EXIT_FAILURE);
}

const char* bb_params_get_string(const char* long_name, char short_name,
                                 const char* help, const char* default_value) {
  const char* val = _bb_params_find_by_name(long_name, short_name, BB_TRUE);
  if (!val) {
    if (default_value == NULL)
      _bb_param_missing(long_name, short_name, _BB_PARAM_STRING, help);
    val = default_value;
  }
  return val;
}

long bb_params_get_int(const char* long_name, char short_name,
                       const char* help, const long* default_value) {
  long int_val;
  char* endp;
  const char* val = _bb_params_find_by_name(long_name, short_name, BB_TRUE);
  if (!val) {
    if (default_value == NULL)
      _bb_param_missing(long_name, short_name, _BB_PARAM_LONG, help);
    int_val = *default_value;
  } else {
    int_val = strtol(val, &endp, 0);
    if (*val == '\0' || *endp != '\0')
      _bb_param_invalid(val, long_name, short_name,
                        _BB_PARAM_LONG, help, default_value);
  }
  return int_val;
}

double bb_params_get_float(const char* long_name, char short_name,
                           const char* help, const double* default_value) {
  double float_val;
  char* endp;
  const char* val = _bb_params_find_by_name(long_name, short_name, BB_TRUE);
  if (!val) {
    if (default_value == NULL)
      _bb_param_missing(long_name, short_name, _BB_PARAM_DOUBLE, help);
    float_val = *default_value;
  } else {
    float_val = strtod(val, &endp);
    if (*val == '\0' || *endp != '\0')
      _bb_param_invalid(val, long_name, short_name,
                        _BB_PARAM_DOUBLE, help, default_value);
  }
  return float_val;
}

int bb_params_get_switch(const char* long_name, char short_name,
                         const char* help, int default_value) {
  long int_val;
  char* endp;
  const char* val = _bb_params_find_by_name(long_name, short_name, BB_FALSE);

  bb_assert(default_value == BB_TRUE || default_value == BB_FALSE);

  // Switch is not present, return the default value.
  if (!val)
    return default_value;

  // Switch is present, but has no value. Switch the default value anyways.
  if (*val == '\0')
    return !default_value;

  int_val = strtol(val, &endp, 10);
  // Value is a number.
  if (endp != NULL && *endp == '\0')
    return int_val == 0 ? BB_FALSE : BB_TRUE;
  // Value is a string.
  if (!strcasecmp(val, "yes") || !strcasecmp(val, "true"))
    return BB_TRUE;
  else if (!strcasecmp(val, "no") || !strcasecmp(val, "false"))
    return BB_FALSE;

  _bb_param_invalid(val, long_name, short_name,
                    _BB_PARAM_SWITCH, help, &default_value);
  // NOTE: Unreachable.
  return BB_FALSE;
}

static const char** _bb_params_clone_list(char** list) {
  const char** clone;
  size_t i = 0;
  // Count elements in list.
  while (list[i++])
    ;
  // Allocate new list.
  clone = bb_malloc(sizeof(*clone) * (i + 1));
  // Set NULL ptr terminator.
  clone[i] = NULL;
  // Copy over the values.
  for (i = 0; list[i]; ++i)
    clone[i] = bb_strdup(list[i]);
  return clone;
}

static _bb_params_t _bb_params_from(int argc, char** argv, char** envp) {
  _bb_params_t params = bb_malloc(sizeof(*params));

  params->argc = argc;
  // NOTE: This pointers are weird, because I don't want the user
  //       to edit the data they point to, so I cast them into
  //       pointers to const data.
  params->argv = _bb_params_clone_list(argv);
  params->envp = _bb_params_clone_list(envp);

  return params;
}

static void _bb_set_ref_time(void) {
  int force_update;

  _bb_ref_time = bb_params_get_int("ref-time", 0,
                                   "Specifies a reference time to use for"
                                   "determining modified files.",
                                   &_bb_ref_time);

  force_update = bb_params_get_switch("force-update", 0,
                                      "Forces all files to be "
                                      "considered modified.",
                                      BB_FALSE);
  if (force_update)
    _bb_ref_time = 0;
}

int main(int argc, char** argv, char** envp) {
  int rc;
  bb_assert(argc >= 1);
  _bb_rebuild_if_needed(argv);
  params = _bb_params_from(argc, argv, envp);
  _bb_set_ref_time();
  rc = bb_main();
  _bb_touch_self(argv[0]);
  return rc;
}

#endif

#endif
