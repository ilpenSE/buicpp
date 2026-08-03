# ICL - ilpeN's C/C++ Library

> [!CAUTION]
> This library and this documentation are unfinished yet.

- This library brings building C++ code or run commands in C++ without any CMake or Makefile
- Although, it's NOT COMPLETED yet, it can do things
- But it's limited now for example you cannot have dependencies or capturing stdout/stderr
- Everything is under icl namespace except u/i X style integer types.

# API
- This section is unfinished

## io sub-namespace
This namespace is related to filesystem

- ### `icl::Result<std::string> read_file_content(const char* file_path, size_t file_size = 0);`

  #### Description
  - Reads whole content of the file from the provided file path

  #### Parameters
  - `const char* file_path`: Path of the file that's gonna be read
  - `size_t file_size`: Size of the file if you don't know the size dont provide anything or provide zero to automatically get size using fseek and ftell

  #### Return Value
  - Whole content of file in std::string
  - Wrapped with Result to indicate errors.
  - On stdio errors, it'll return error struct and code field is errno, msg field is a function failed

- ### `icl::Result<icl::io::File> read_entire_file(const char* file_path);`

  #### Description
  - Reads whole content of the file from the provided file path

  #### Parameters
  - `const char* file_path`: Path of the file that's gonna be read

  #### Return Value
  - Whole content of file in std::string
  - Wrapped with Result to indicate errors.
  - On stdio errors, it'll return error struct and code field is errno, msg field is a function failed

# About

- ilpeN
