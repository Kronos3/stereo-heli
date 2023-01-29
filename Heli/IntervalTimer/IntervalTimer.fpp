module Heli {

  port StopTimer()
  port StartTimer(
    ref interval: Fw.Time,  @< Periodic interval to trigger cycle
    ref value: Fw.Time      @< Offset from current time to trigger cycle
  )

  @ A Linux interval timer
  passive component IntervalTimer {

    @ Cycle output
    output port CycleOut: Svc.Cycle

    sync input port start: StartTimer
    sync input port stop: StopTimer

    @ Event port
    event port Log

    @ Text event port
    text event port LogText

    @ Time get port
    time get port Time

    event AlreadyRunning() \
        severity warning high \
        format "Interval timer is already running"

    event NotRunning() \
        severity warning high \
        format "Interval timer is not running"

    event Started() \
        severity activity low \
        format "Interval timer started"

    event Stopped() \
        severity activity low \
        format "Interval timer stopped"

    event MissedCycles(nCycles: U32) \
        severity warning low \
        format "Missed {} cycles on interval timer"
  }
}
