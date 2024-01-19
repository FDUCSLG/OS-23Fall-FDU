get_property(kernel_image GLOBAL PROPERTY kernel_image_path)

set(boot_files
    "${kernel_image}"
    "armstub8-rpi4.bin"
    "bootcode.bin"
    "config.txt"
    "COPYING.linux"
    "fixup_cd.dat"
    "fixup.dat"
    "fixup4.dat"
    "fixup4cd.dat"
    "LICENCE.broadcom"
    "start_cd.elf"
    "start.elf"
    "start4.elf"
    "start4cd.elf")

set(user_files "init" 
"cat"
"sh"
"echo"
"ls"
"mkfs"
"mkdir"
"usertests")

foreach(file ${user_files})
    list(APPEND bin_list ../src/user/${file})
endforeach()
    
add_custom_command(
    OUTPUT sd.img
    BYPRODUCTS boot.img
    COMMAND ./generate-image.py ${CMAKE_CURRENT_BINARY_DIR} ${boot_files} ${user_files}
    # bugfix: CMake won't rebuild sd.img when user_bin changes since it is a target-
    # level dependency. This is a workaround that adds the binaries (i.e. bin_list) as
    # file-level dependencies too.
    DEPENDS kernel generate-image.py ${boot_files} user_bin ${bin_list}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(image ALL DEPENDS sd.img)

set_property(GLOBAL PROPERTY sd_image_path ${CMAKE_CURRENT_BINARY_DIR}/sd.img)
