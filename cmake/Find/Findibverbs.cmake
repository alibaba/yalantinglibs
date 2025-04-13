find_path(IBVERBS_INCLUDE_DIRS
  NAMES infiniband/verbs.h
  HINTS
  ${IBVERBS_INCLUDE_DIR}
  ${IBVERBS_ROOT_DIR}
  ${IBVERBS_ROOT_DIR}/include)

find_library(IBVERBS_LIBRARIES
  NAMES ibverbs
  HINTS
  ${IBVERBS_LIB_DIR}
  ${IBVERBS_ROOT_DIR}
  ${IBVERBS_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ibverbs DEFAULT_MSG IBVERBS_INCLUDE_DIRS IBVERBS_LIBRARIES)
mark_as_advanced(IBVERBS_INCLUDE_DIR IBVERBS_LIBRARIES)
