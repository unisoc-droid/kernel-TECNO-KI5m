# Kernel source for TECNO KI5m (LineageOS 20)

This tree builds the KI5m kernel from source via LineageOS `TARGET_KERNEL_SOURCE`.

- Base defconfig: `arch/arm64/configs/gki_defconfig`
- KI5m fragment: `arch/arm64/configs/ki5m_early.config` (kernel `%.config`
  merge target; Lineage passes `TARGET_KERNEL_CONFIG` straight to `make`)
- Device tree selects both through `TARGET_KERNEL_CONFIG`.
- Module load lists: `android/ki5m_first_stage_modules` and
  `android/ki5m_vendor_modules`.
- Custom `build.config.ki5m.aarch64` remains available for standalone kernel
  distribution builds, but the ROM build does not require a prebuilt kernel.

No kernel image or modules are committed here. No build has been run by the
source-preparation agent.
