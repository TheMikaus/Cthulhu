# Cthulhu HOME Menu extension framework

Version `0.7.1-diagnostic` is the read-only framework milestone. It hooks
the two verified USA HOME Menu layout-processing call sites and publishes
validated layout snapshots through a small versioned plugin API.

The current signature set supports only the captured USA HOME Menu build with
title ID `0004003000008F02`. The framework refuses to install hooks if the title
ID or any of six ARM instructions differ. Each captured layout is validated
before it is dispatched.

The current startup sequence is:

1. Validate the exact HOME Menu title and hook signatures.
2. Register the built-in layout diagnostic against framework API version 1.
3. Run Nintendo's original NAND/SD processors from the hook callbacks.
4. Validate and publish read-only title ID, slot, position, and folder snapshots.
5. Write a bounded log to `sd:/3ds/Cthulhu/home-menu-framework.log`.

This build performs no sorting, savedata writes, or other layout mutations. Its
purpose is to verify the event boundary that sorting, input, search, drawing,
navigation, and launching plugins will share.

Build with `build_home_menu_plugin.ps1`. Install the resulting file as:

`sd:/luma/plugins/0004003000008F02/CthulhuHomeMenu.3gx`

Luma's plugin loader must be enabled. If HOME Menu fails to start, boot while
holding SELECT and disable the plugin loader, or remove the plugin file from the
SD card.
