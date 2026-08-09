# Keep all native libraries and their exported methods
# Prevent native class stripping for XEMU emulation code
keep class com.izzy2lost.x1box.* { *; }
keep class **/*.* { *; }
