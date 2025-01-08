
function(add_cute_ext_test test_name source_files)
    message("Registering test ${test_name} (${source_files})")
    add_executable(${test_name} "${source_files};${CMAKE_CURRENT_LIST_DIR}/extern_testsuite.cpp")
    set_target_properties(${test_name} PROPERTIES OUTPUT_NAME ${test_name})

    target_link_libraries(${test_name} cute_ext::cute_ext cxxopts tiny-process-library::tiny-process-library termcolor::termcolor Boost::system  Boost::filesystem)
    
    add_test(
        NAME ${test_name} 
        COMMAND "$<TARGET_FILE:${test_name}>"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}   # wd = project binary dir root so we can find goldilock exe    
    )

endfunction()

set(test_source_files
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
)

foreach(test_file IN LISTS test_source_files)
    get_filename_component(test_name ${test_file} NAME_WE) # without .cpp extension
    add_cute_ext_test(${test_name} "${test_file}")    
endforeach()
