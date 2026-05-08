/**
 *
 *  @file AtomicFile.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Atomic file write helper implementation
 *
 */

#include <vix/kv/utils/AtomicFile.hpp>

#include <fstream>
#include <system_error>

namespace vix::kv::utils
{
  core::KvResult<void> AtomicFile::write(
      const std::filesystem::path &target_path,
      const Bytes &bytes)
  {
    if (target_path.empty())
    {
      return core::KvResult<void>::err(
          core::KvError::invalid_argument(
              "target path must not be empty"));
    }

    auto directory = ensure_parent_directory(target_path);

    if (directory.is_err())
    {
      return directory;
    }

    const auto temp_path = temporary_path(target_path);

    auto written = write_direct(temp_path, bytes);

    if (written.is_err())
    {
      return written;
    }

    std::error_code ec;

    if (std::filesystem::exists(target_path, ec))
    {
      std::filesystem::remove(target_path, ec);

      if (ec)
      {
        return core::KvResult<void>::err(
            core::KvError::make(
                core::KvErrorCode::IoError,
                "failed to remove existing target file",
                target_path,
                ec.message()));
      }
    }

    std::filesystem::rename(temp_path, target_path, ec);

    if (ec)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to rename temporary file to target path",
              target_path,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> AtomicFile::write_string(
      const std::filesystem::path &target_path,
      std::string_view text)
  {
    Bytes bytes(text.begin(), text.end());

    return write(target_path, bytes);
  }

  core::KvResult<AtomicFile::Bytes>
  AtomicFile::read(const std::filesystem::path &path)
  {
    if (path.empty())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::invalid_argument(
              "file path must not be empty"));
    }

    if (!std::filesystem::exists(path))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::NotFound,
              "file does not exist",
              path));
    }

    if (!std::filesystem::is_regular_file(path))
    {
      return core::KvResult<Bytes>::err(
          core::KvError::io(
              "path exists but is not a regular file",
              path));
    }

    std::error_code ec;
    const auto file_size = std::filesystem::file_size(path, ec);

    if (ec)
    {
      return core::KvResult<Bytes>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to read file size",
              path,
              ec.message()));
    }

    Bytes bytes(static_cast<std::size_t>(file_size));

    std::ifstream stream(path, std::ios::binary | std::ios::in);

    if (!stream.is_open())
    {
      return core::KvResult<Bytes>::err(
          core::KvError::io(
              "failed to open file for reading",
              path));
    }

    if (!bytes.empty())
    {
      stream.read(
          reinterpret_cast<char *>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));

      if (!stream)
      {
        return core::KvResult<Bytes>::err(
            core::KvError::io(
                "failed to read file",
                path));
      }
    }

    return core::KvResult<Bytes>::ok(std::move(bytes));
  }

  core::KvResult<std::string>
  AtomicFile::read_string(const std::filesystem::path &path)
  {
    auto bytes = read(path);

    if (bytes.is_err())
    {
      return core::KvResult<std::string>::err(bytes.error());
    }

    const auto &buffer = bytes.value();

    std::string text(buffer.begin(), buffer.end());

    return core::KvResult<std::string>::ok(std::move(text));
  }

  std::filesystem::path AtomicFile::temporary_path(
      const std::filesystem::path &target_path)
  {
    return target_path.string() + ".tmp";
  }

  core::KvResult<void> AtomicFile::ensure_parent_directory(
      const std::filesystem::path &target_path)
  {
    const auto parent = target_path.parent_path();

    if (parent.empty())
    {
      return core::KvResult<void>::ok();
    }

    if (std::filesystem::exists(parent))
    {
      if (!std::filesystem::is_directory(parent))
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "parent path exists but is not a directory",
                parent));
      }

      return core::KvResult<void>::ok();
    }

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);

    if (ec)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::IoError,
              "failed to create parent directory",
              parent,
              ec.message()));
    }

    return core::KvResult<void>::ok();
  }

  core::KvResult<void> AtomicFile::write_direct(
      const std::filesystem::path &path,
      const Bytes &bytes)
  {
    std::ofstream stream(
        path,
        std::ios::binary |
            std::ios::out |
            std::ios::trunc);

    if (!stream.is_open())
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to open file for writing",
              path));
    }

    if (!bytes.empty())
    {
      stream.write(
          reinterpret_cast<const char *>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));

      if (!stream)
      {
        return core::KvResult<void>::err(
            core::KvError::io(
                "failed to write file",
                path));
      }
    }

    stream.flush();

    if (!stream)
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to flush file",
              path));
    }

    stream.close();

    if (stream.fail())
    {
      return core::KvResult<void>::err(
          core::KvError::io(
              "failed to close file",
              path));
    }

    return core::KvResult<void>::ok();
  }

} // namespace vix::kv::utils
