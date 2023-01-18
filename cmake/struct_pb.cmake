function(protobuf_generate_modified)
    find_package(Protobuf REQUIRED)
    set(_options APPEND_PATH DESCRIPTORS)
    set(_singleargs LANGUAGE OUT_VAR EXPORT_MACRO PROTOC_OUT_DIR PLUGIN PROTOC_OPTION)
    if(COMMAND target_sources)
        list(APPEND _singleargs TARGET)
    endif()
    set(_multiargs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS)

    cmake_parse_arguments(protobuf_generate "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

    if(NOT protobuf_generate_PROTOS AND NOT protobuf_generate_TARGET)
        message(SEND_ERROR "Error: protobuf_generate called without any targets or source files")
        return()
    endif()

    if(NOT protobuf_generate_OUT_VAR AND NOT protobuf_generate_TARGET)
        message(SEND_ERROR "Error: protobuf_generate called without a target or output variable")
        return()
    endif()

    if(NOT protobuf_generate_LANGUAGE)
        set(protobuf_generate_LANGUAGE struct_pb)
    endif()
    string(TOLOWER ${protobuf_generate_LANGUAGE} protobuf_generate_LANGUAGE)

    if(NOT protobuf_generate_PROTOC_OUT_DIR)
        set(protobuf_generate_PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    set(_opt)
    if(protobuf_generate_PROTOC_OPTION)
        set(_opt "${protobuf_generate_PROTOC_OPTION}")
    endif()
    if(protobuf_generate_EXPORT_MACRO AND protobuf_generate_LANGUAGE STREQUAL cpp)
        set(_opt "${opt},dllexport_decl=${protobuf_generate_EXPORT_MACRO}")
    endif()
    set(_opt "${_opt}:")
    if(protobuf_generate_PLUGIN)
        set(_plugin "--plugin=${protobuf_generate_PLUGIN}")
    endif()

    if(NOT protobuf_generate_GENERATE_EXTENSIONS)
        if(protobuf_generate_LANGUAGE STREQUAL cpp)
            set(protobuf_generate_GENERATE_EXTENSIONS .pb.h .pb.cc)
        elseif(protobuf_generate_LANGUAGE STREQUAL python)
            set(protobuf_generate_GENERATE_EXTENSIONS _pb2.py)
        elseif(protobuf_generate_LANGUAGE STREQUAL struct_pb)
            set(protobuf_generate_GENERATE_EXTENSIONS .struct_pb.h .struct_pb.cc)
        else()
            message(SEND_ERROR "Error: protobuf_generate given unknown Language ${LANGUAGE}, please provide a value for GENERATE_EXTENSIONS")
            return()
        endif()
    endif()

    if(protobuf_generate_TARGET)
        get_target_property(_source_list ${protobuf_generate_TARGET} SOURCES)
        foreach(_file ${_source_list})
            if(_file MATCHES "proto$")
                list(APPEND protobuf_generate_PROTOS ${_file})
            endif()
        endforeach()
    endif()

    if(NOT protobuf_generate_PROTOS)
        message(SEND_ERROR "Error: protobuf_generate could not find any .proto files")
        return()
    endif()

    if(protobuf_generate_APPEND_PATH)
        # Create an include path for each file specified
        foreach(_file ${protobuf_generate_PROTOS})
            get_filename_component(_abs_file ${_file} ABSOLUTE)
            get_filename_component(_abs_path ${_abs_file} PATH)
            list(FIND _protobuf_include_path ${_abs_path} _contains_already)
            if(${_contains_already} EQUAL -1)
                list(APPEND _protobuf_include_path -I ${_abs_path})
            endif()
        endforeach()
    else()
        set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    foreach(DIR ${protobuf_generate_IMPORT_DIRS})
        get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
        list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
        if(${_contains_already} EQUAL -1)
            list(APPEND _protobuf_include_path -I ${ABS_PATH})
        endif()
    endforeach()

    set(_generated_srcs_all)
    foreach(_proto ${protobuf_generate_PROTOS})
        get_filename_component(_abs_file ${_proto} ABSOLUTE)
        get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
        get_filename_component(_basename ${_proto} NAME_WLE)
        file(RELATIVE_PATH _rel_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_dir})

        set(_possible_rel_dir)
        if (NOT protobuf_generate_APPEND_PATH)
            set(_possible_rel_dir ${_rel_dir}/)
        endif()

        set(_generated_srcs)
        foreach(_ext ${protobuf_generate_GENERATE_EXTENSIONS})
            list(APPEND _generated_srcs "${protobuf_generate_PROTOC_OUT_DIR}/${_possible_rel_dir}${_basename}${_ext}")
        endforeach()

        if(protobuf_generate_DESCRIPTORS AND protobuf_generate_LANGUAGE STREQUAL cpp)
            set(_descriptor_file "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.desc")
            set(_dll_desc_out "--descriptor_set_out=${_descriptor_file}")
            list(APPEND _generated_srcs ${_descriptor_file})
        endif()
        list(APPEND _generated_srcs_all ${_generated_srcs})

        add_custom_command(
                OUTPUT ${_generated_srcs}
                COMMAND  protobuf::protoc
                ARGS --${protobuf_generate_LANGUAGE}_out ${_opt}${protobuf_generate_PROTOC_OUT_DIR} ${_plugin} ${_dll_desc_out} ${_protobuf_include_path} ${_abs_file}
                DEPENDS ${_abs_file} protobuf::protoc
                COMMENT "Running ${protobuf_generate_LANGUAGE} protocol buffer compiler on ${_proto}"
                VERBATIM )
    endforeach()

    set_source_files_properties(${_generated_srcs_all} PROPERTIES GENERATED TRUE)
    if(protobuf_generate_OUT_VAR)
        set(${protobuf_generate_OUT_VAR} ${_generated_srcs_all} PARENT_SCOPE)
    endif()
    if(protobuf_generate_TARGET)
        target_sources(${protobuf_generate_TARGET} PRIVATE ${_generated_srcs_all})
        target_include_directories(${protobuf_generate_TARGET} PUBLIC ${protobuf_generate_PROTOC_OUT_DIR})
    endif()
endfunction()


function(protobuf_generate_struct_pb SRCS HDRS)
    cmake_parse_arguments(protobuf_generate_struct_pb "" "EXPORT_MACRO;DESCRIPTORS;OPTION" "" ${ARGN})

    set(_proto_files "${protobuf_generate_struct_pb_UNPARSED_ARGUMENTS}")
    if(NOT _proto_files)
        message(SEND_ERROR "Error: PROTOBUF_GENERATE_STRUCT_PB() called without any proto files")
        return()
    endif()

    if(PROTOBUF_GENERATE_STRUCT_PB_APPEND_PATH)
        set(_append_arg APPEND_PATH)
    endif()

    if(protobuf_generate_struct_pb_DESCRIPTORS)
        set(_descriptors DESCRIPTORS)
    endif()

    if(protobuf_generate_struct_pb_OPTION)
        set(_opt ${protobuf_generate_struct_pb_OPTION})
    endif()

    if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
        set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
    endif()

    if(DEFINED Protobuf_IMPORT_DIRS)
        set(_import_arg IMPORT_DIRS ${Protobuf_IMPORT_DIRS})
    endif()

    set(_outvar)
    protobuf_generate_modified(${_append_arg} ${_descriptors}
            LANGUAGE struct_pb EXPORT_MACRO ${protobuf_generate_struct_pb_EXPORT_MACRO}
            PLUGIN $<TARGET_FILE:protoc-gen-struct_pb>
            OUT_VAR _outvar ${_import_arg} PROTOS ${_proto_files}
            PROTOC_OPTION ${_opt}
            )

    set(${SRCS})
    set(${HDRS})
    if(protobuf_generate_struct_pb_DESCRIPTORS)
        set(${protobuf_generate_struct_pb_DESCRIPTORS})
    endif()

    foreach(_file ${_outvar})
        if(_file MATCHES "cc$")
            list(APPEND ${SRCS} ${_file})
        elseif(_file MATCHES "desc$")
            list(APPEND ${protobuf_generate_struct_pb_DESCRIPTORS} ${_file})
        else()
            list(APPEND ${HDRS} ${_file})
        endif()
    endforeach()
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
    if(protobuf_generate_struct_pb_DESCRIPTORS)
        set(${protobuf_generate_struct_pb_DESCRIPTORS} "${${protobuf_generate_struct_pb_DESCRIPTORS}}" PARENT_SCOPE)
    endif()
endfunction()

function(target_protos_struct_pb target)
    set(_options APPEND_PATH DESCRIPTORS)
    set(_singleargs LANGUAGE EXPORT_MACRO PROTOC_OUT_DIR PLUGIN OPTION)
    set(_multiargs IMPORT_DIRS PRIVATE PUBLIC)
    cmake_parse_arguments(target_protos_struct_pb "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")
    set(_proto_files "${target_protos_struct_pb_PRIVATE}" "${target_protos_struct_pb_PUBLIC}")
    if (NOT _proto_files)
        message(SEND_ERROR "Error: TARGET_PROTOS_STRUCT_PB() called without any proto files")
        return()
    endif ()

    if (DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
        set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
    endif ()

    if (DEFINED Protobuf_IMPORT_DIRS)
        set(_import_arg IMPORT_DIRS ${Protobuf_IMPORT_DIRS})
    endif ()

    if (target_protos_struct_pb_OPTION)
        set(_opt ${target_protos_struct_pb_OPTION})
    endif ()
    protobuf_generate_modified(
            TARGET ${target}
            LANGUAGE struct_pb EXPORT_MACRO ${target_protos_struct_pb_EXPORT_MACRO}
            PLUGIN $<TARGET_FILE:protoc-gen-struct_pb>
            ${_import_arg} PROTOS ${_proto_files}
            PROTOC_OPTION ${_opt}
    )
endfunction()
