
@PACKAGE_INIT@

find_package(GerHobbelt_tiny-process-library CONFIG REQUIRED)
find_package(ikalnytskyi_termcolor CONFIG REQUIRED)
find_package(sailormoon_flags CONFIG REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/@targets_export_name@.cmake")
check_required_components("@PROJECT_NAME@")
