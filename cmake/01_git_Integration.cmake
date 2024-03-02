# idea shamelessly taken from: http://xit0.org/2013/04/cmake-use-git-branch-and-commit-details-in-project/
# with some additions, e.g. for finding git, checking if source directory is git directory, renaming variables to VCS

# initialize commit variables 
set(GIT_BRANCH_DEFAULT "unknown")
set(GIT_COMMIT_ID_DEFAULT "0000000")
set(VCS_BRANCH ${GIT_BRANCH_DEFAULT})
set(VCS_COMMIT_ID ${GIT_COMMIT_ID_DEFAULT})

# we need the local systems git
find_package(Git REQUIRED)

# if we have git ...
if (GIT_FOUND)
    # is the source directory under git version control at all?
    execute_process(
      COMMAND ${GIT_EXECUTABLE} status
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE VCS_IS_GIT
      OUTPUT_QUIET
      ERROR_QUIET
    )
    
    # only if we are under git version control...
    if(VCS_IS_GIT EQUAL 0)       
        # Get the current working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE VCS_BRANCH
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Get the latest abbreviated commit hash of the working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE VCS_COMMIT_ID
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        if (VCS_BRANCH STREQUAL GIT_BRANCH_DEFAULT OR VCS_COMMIT_ID STREQUAL GIT_COMMIT_ID_DEFAULT)
            message(WARNING "! Not building sources from git repository. You need to provide VCS_BRANCH and VCS_COMMIT_ID manually.")
        endif()
    endif()

    # output the version information we are on
    message(STATUS "Processing sources from '${CMAKE_SOURCE_DIR}' provided by git")
    message(STATUS "  on branch: ${VCS_BRANCH}")
    message(STATUS "  revision:  ${VCS_COMMIT_ID}")
# no git -> no versioning
else()
    message(WARNING "! No git executable found. Unable to determine VCS_BRANCH and VCS_COMMIT_ID. Provide them manually.")
endif()


# a helper function to add VCS information to the FILE_NAME passed
function(_add_vcs_info_to_file FILE_NAME)
    set_property(
        SOURCE ${FILE_NAME}
        APPEND
        PROPERTY COMPILE_DEFINITIONS
        GIT_BRANCH="${VCS_BRANCH}" GIT_VERSION="${VCS_COMMIT_ID}"
    )
endfunction()
