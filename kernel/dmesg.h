void dmesg_init();

/// Append a message to diagnostic messages buffer.
/// Should be called only by kernel.
///
/// Message is formatted just like printf.
/// Supported specifiers are %d, %x, %p, %s.
void pr_msg(const char *format, ...);

