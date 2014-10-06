Configuration files are at settings/logs/logs.conf

* NEW: log {} OLD: N/A Tells the ircd where and what to log(s). You can have
* log "log file"

The following logs will be created and by default,  reset once they reach 5mb size

log "/home/ircd/unreal/logs/error.log"
log "/home/ircd/unreal/logs/kills.log"
log "/home/ircd/unreal/logs/tkl.log" 
log "/home/ircd/unreal/logs/connects.log"
log "/home/ircd/unreal/logs/server-connects.log" 
log "/home/ircd/unreal/logs/kline.log" 
log "/home/ircd/unreal/logs/oper.log" 
log "/home/ircd/unreal/logs/sadmin-commands.log" 
log "/home/ircd/unreal/logs/chg-commands.log" 
log "/home/ircd/unreal/logs/oper-override.log" 
log "/home/ircd/unreal/logs/spamfilter.log" 
