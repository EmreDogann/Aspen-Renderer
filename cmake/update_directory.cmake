# Modified from: https://bravenewmethod.com/2017/06/18/update_directory-command-for-cmake/
# cmake/update_directory.cmake
file(GLOB_RECURSE _file_list RELATIVE "${source_dir}" "${source_dir}/*")

# Iterate through every file in the specified folder.
foreach(each_file ${_file_list})
  set(destinationfile "${dest_dir}/${each_file}")
  set(sourcefile "${source_dir}/${each_file}")

  # If the file does not exist, copy it.
  if(NOT EXISTS ${destinationfile})
    get_filename_component(destinationdir ${destinationfile} DIRECTORY)
    file(COPY ${sourcefile} DESTINATION ${destinationdir})
    message(STATUS "Copying file: ${sourcefile}")
  else()
    # Comput the SHA256 hashes of both the source and destination file.
    file(SHA256 ${destinationfile} destination_checksum)
    file(SHA256 ${sourcefile} source_checksum)

    # Compare the hash values. If the hashes don't match, overwrite the destination file with the source file.
    if (NOT ${source_checksum} EQUAL ${destination_checksum})
      get_filename_component(destinationdir ${destinationfile} DIRECTORY)
      file(COPY ${sourcefile} DESTINATION ${destinationdir})
      message(STATUS "Updating file: ${sourcefile}")
    endif()
  endif()
endforeach(each_file)