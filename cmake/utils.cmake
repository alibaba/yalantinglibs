macro(check_asan _RESULT)
    include(CheckCXXSourceRuns)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    check_cxx_source_runs(
            [====[
int main()
{
  return 0;
}
]====]
            ${_RESULT}
    )
    unset(CMAKE_REQUIRED_FLAGS)
endmacro()

macro(get_git_hash _git_hash)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      OUTPUT_VARIABLE ${_git_hash}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endif()
endmacro()

macro(get_git_branch _git_branch)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      OUTPUT_VARIABLE ${_git_branch}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endif()
endmacro()
