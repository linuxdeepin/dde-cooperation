# Try to find displayjack_kvm library and include path.
# Once done this will define
#
# LIBDJK_FOUND
# LIBDJK_INCLUDE_DIR
# LIBDJK_LIBRARIES

find_path(LIBDJK_INCLUDE_DIR displayjack_kvm/displayjack_kvm.h)
find_library(LIBDJK_LIBRARY displayjack_kvm)
message("LibDisplayjackKvm: ${LIBDJK_LIBRARY}")
message("LibDisplayjackKvm include dir: ${LIBDJK_INCLUDE_DIR}")

# Handle the REQUIRED argument and set LIBDJK_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDJK DEFAULT_MSG LIBDJK_LIBRARY LIBDJK_INCLUDE_DIR)

mark_as_advanced(LIBDJK_INCLUDE_DIR)
mark_as_advanced(LIBDJK_LIBRARY)
message("LibDisplayjackKvm found: ${LIBDJK_FOUND}")

if(LIBDJK_FOUND)
  add_definitions(-DLIBDJK_SUPPORT)
  set(LIBDJK_LIBRARIES ${LIBDJK_LIBRARY})
  message("LibDisplayjackKvm found. ${LIBDJK_LIBRARIES}")
endif()
