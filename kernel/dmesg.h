void dmesg_init();

///
/// dmesg_log*: automatic logging of kernel events to dmesg buffer.
///
/// @{

enum dmesg_log_eventclass {
  DMESG_LOG_INTERRUPT = 0,
  DMESG_LOG_PROCSWITCH = 1,
  DMESG_LOG_SYSCALL = 2,
  DMESG_LOG_EVENTCLASS_COUNT, ///< Fake eventclass, equals the number of real
                              ///< eventclasses.
};

/// Log message if logging is enabled for this \p eventclass.
///
/// Message is formatted just like printf.
/// Supported specifiers are %d, %x, %p, %s.
void dmesg_log(enum dmesg_log_eventclass eventclass, const char *format, ...);

/// Toggle dmesg logging for a \p eventclass.
///
/// If \p `duration_ticks > 0`, logging is enabled only for \p duration_ticks
/// ticks. If \p `duartion_ticks = 0`, logging is enabled permanently.
/// If \p `duration_ticks < 0`, logging is disabled.
void dmesg_log_eventclass_toggle(enum dmesg_log_eventclass eventclass,
                                 int duration_ticks);

/// @}

/// Append a message to diagnostic messages buffer.
/// Should be called only by kernel.
///
/// Message is formatted just like printf.
/// Supported specifiers are %d, %x, %p, %s.
void pr_msg(const char *format, ...);
