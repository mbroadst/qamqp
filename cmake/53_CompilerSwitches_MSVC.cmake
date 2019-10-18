# disable manifest generation
# read here: http://stackoverflow.com/a/8359871
set(CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO)
set(CMAKE_MODULE_LINKER_FLAGS /MANIFEST:NO)
set(CMAKE_SHARED_LINKER_FLAGS /MANIFEST:NO)
