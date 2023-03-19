void dmesg_init();

/// Append message \p str to diagnostic messages buffer.
///
/// Should be called by kernel.
void pr_msg(const char *str);

