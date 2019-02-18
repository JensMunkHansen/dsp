#ifndef SLURP_H
#define SLURP_H

class Slurp {
	void *begin_{ nullptr };
	void *end_;
public:
	Slurp() = delete;
	Slurp(const Slurp &) = delete;
	Slurp &operator=(Slurp) = delete;

	Slurp(Slurp &&other) noexcept = default;
	Slurp &operator=(Slurp &&other) noexcept = default;

	[[gnu::nonnull]] Slurp(const char *path);

	~Slurp();

	[[gnu::const, gnu::returns_nonnull]] const char *begin() const noexcept;
	[[gnu::const, gnu::returns_nonnull]] const char *end() const noexcept;
};

#endif //SLURP_H

// Slurp.cc
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <system_error>

using namespace std;

#define ERROR_IF(CONDITION) do {\
		if (__builtin_expect(CONDITION, false))\
			throw system_error(errno, system_category());\
	} while (false)

#define ERROR_IF_POSIX(POSIX_PREFIXED_FUNCTION_CALL) do {\
		const int error_number{ posix_##POSIX_PREFIXED_FUNCTION_CALL };\
		if (__builtin_expect(error_number, false))\
			throw system_error(error_number, system_category());\
	} while (false)

Slurp::Slurp(const char *path)
{
	class FileDescriptor {
		const int fd_;
	public:
		FileDescriptor(int fd) : fd_{ fd } {ERROR_IF(fd_ == -1);}
		~FileDescriptor() {close(fd_);}
		[[gnu::const]] operator int() const noexcept {return fd_;}
	};

	FileDescriptor fd{ open(path, O_RDONLY) };

	ERROR_IF_POSIX(fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL));

	struct stat file_statistics;

	ERROR_IF(fstat(fd, &file_statistics));

	const size_t blksize{ static_cast<size_t>(file_statistics.st_blksize) };
	const size_t bufsize{
		static_cast<size_t>(file_statistics.st_size) + sizeof(char32_t)
	};
	ERROR_IF_POSIX(memalign(&begin_, blksize, bufsize));

	end_ = static_cast<char*>(begin_) + file_statistics.st_size;

	ERROR_IF_POSIX(madvise(begin_, bufsize, POSIX_MADV_SEQUENTIAL));

	ERROR_IF(read(fd, begin_, file_statistics.st_size) == -1);

	*static_cast<char32_t*>(end_) = U'\0'; // Ensure null termination
}

Slurp::~Slurp() {free(begin_);}

const char *Slurp::begin() const noexcept {return static_cast<char*>(begin_);}
const char *Slurp::end() const noexcept {return static_cast<char*>(end_);}
