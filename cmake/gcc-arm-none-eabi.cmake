set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
# arm-none-eabi- must be part of path environment
set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wconversion -Wsign-conversion -Wsign-compare -Wundef") # << сюда добавляем свои





set(CMAKE_C_FLAGS_DEBUG "-O0 -g3") # -O0
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3") # -O0
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics") #?? cxx<->c

# // TODO add flag? vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsingle-precision-constant) #?? for float ??????

set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H723XG_FLASH.ld\"")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")










# # Расширенные предупреждения для максимального выявления проблем
# set(EXTENDED_WARNINGS 
#     # ""
#     "-Wundef -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wcast-align=strict -Wcast-qual -Wbad-function-cast -Wwrite-strings -Wstrict-aliasing=2 -Wdate-time -Wfloat-equal -Wlogical-op -Wstrict-overflow=5 -Wformat=2 -Wformat-nonliteral -Wformat-security -Winit-self -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wunused -Wuninitialized -Wshadow -Wpointer-arith -Wduplicated-cond -Wduplicated-branches -Wnull-dereference -Walloc-zero -Wvla -Woverlength-strings -Wdouble-promotion -Wjump-misses-init -Wmissing-field-initializers"
# )

# # Для GCC 10+ добавить статический анализатор
# if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0")
#     set(EXTENDED_WARNINGS "${EXTENDED_WARNINGS} -fanalyzer")
# endif()

# # Дополнительные флаги для C++
# set(CXX_EXTENDED_WARNINGS
#     "-Wctor-dtor-privacy -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wsign-promo -Weffc++ -Wstrict-null-sentinel -Wnoexcept -Wdelete-non-virtual-dtor -Wnarrowing -Wclass-memaccess -Wcatch-value -Wextra-semi"
# )

# # Применение флагов
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTENDED_WARNINGS}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTENDED_WARNINGS} ${CXX_EXTENDED_WARNINGS}")
