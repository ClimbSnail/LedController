set(sources "main.c")
set(includes ".")

# file(GLOB sources "./*.c*")
file(GLOB_RECURSE sources "./*.c*")

# list(APPEND sources ./*.c*)
list(APPEND includes "sh_hal" "sh_driver" "controller")

idf_component_register(SRCS ${sources} INCLUDE_DIRS ${includes}
    EMBED_FILES "page/web_page.html")

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_FONT_MONTSERRAT_10=1")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_FONT_MONTSERRAT_12=1")

message(STATUS SH_SW_v${SH_SW_VER})