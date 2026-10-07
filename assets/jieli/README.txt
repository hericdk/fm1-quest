Three files of the JieLi AC79 SDK (AC79NN_SDK_V1.2.1_2023-12-13, Apache License 2.0, https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK)
that tools/build.py needs to make the installable package: uboot.boot (cpu/wl82/tools), cfg_tool.bin (cpu/wl82/tools),
eq_cfg_hw.bin (cpu/wl82/tools/cfg). build.py checks their SHA-256 (SDK_SHA256). They are kept here so that CI does not depend
on gitee being reachable; the CI copies them to $HOME/fw-AC79_AIoT_SDK/cpu/wl82/tools/.
