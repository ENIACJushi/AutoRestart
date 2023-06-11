// ==================== Title ====================

  /* ---------------------------------------- *\
   *  Name        :  AutoRestart              *
   *  Description :  自动重启                  *
   *  Version     :  0.1.3                    *
   *  Author      :  ENIAC_Jushi              *
  \* ---------------------------------------- */

// ================== Tool ==================
function isJsonEmpty(obj){
    for(var key in obj) {
        return false;
    }
    return true;
}

function kickALL(){
    var playerList = mc.getOnlinePlayers();
    for (var pl of playerList) {
        pl.kick("Server stopping..");
    }
}

// ================== Initialize ===================
let version = 0.1
const PATH = "plugins/AutoRestart";
// Logger output
logger.setConsole(true);
logger.setTitle('AutoRestart');
logger.info('AutoRestart is running');

// Config
var Config;
function loadConfig(){
    var defaultConfig = {
        "restart_enable": true,
        "timeout"       : 30,
        "hide_window"   : true,
        "scan_interval" : 15,
        "close_timeout" : 30,
        "start_timeout" : 60,
        "vote_enable"   : true,
        "vote_percent"  : 0.66,
        "vote_timeout"  : 300
    }
    Config = JSON.parse(file.readFrom(PATH + "/Config.json"));
    var rewrite = false;
    for(var key in defaultConfig){
        if(Config[key] == null){
            Config[key] = defaultConfig[key];
            rewrite = true;
        }
    }
    if(rewrite){
        file.writeTo(PATH + "/Config.json", JSON.stringify(Config , null , '\t'));
    }
}
loadConfig();

// 加载定时重启列表
if(!file.exists(PATH + "/RestartTask.json")) {
    file.writeTo(PATH + "/RestartTask.json", JSON.stringify([
        {
            "type": "Timeout", // 任务加载后多少秒重启
            "time": 60,
            "message": "", // 重启信息
            "enable": false
        },
        {
            "type": "Time", // 到达设定时刻重启,格式: 周-小时:分钟, 若不设置周则每天都重启
            "time": "6-23:59",
            "message": "", // 重启信息
            "enable": false
        }
    ] , null , '\t'));
}

var AutoRestartTask = JSON.parse(file.readFrom(PATH + "/RestartTask.json"));

// 加载重启任务列表
var restartTaskID = -1;
function loadAutoRestartTask(){
    var myDate = new Date();
    var now_day = myDate.getDay(); // 获取星期(1 ~ 7)
    var now_hours = myDate.getHours(); // 获取小时(0 ~ 23)
    var now_minutes = myDate.getMinutes(); // 获取分钟(0-59)
    var min_timeout = -1;
    for(var task of AutoRestartTask){
        if(!task["enable"]) continue;
        let time;
        if(task["type"] == "Timeout"){
            time = task["time"];
        }
        else if(task["type"] == "Time"){
            time = 0;
            // 由字符串获得任务时间(按分钟计算)
            let timeStr = task["time"].split("-");
            let task_day;
            let task_hours;
            let task_minutes;
            if(timeStr.length == 2){
                task_day = parseInt(timeStr[0]);
            }
            else{
                task_day = -1;
            }
            timeStr = timeStr[timeStr.length - 1].split(':');
            task_hours = parseInt(timeStr[0]);
            task_minutes = parseInt(timeStr[1]);
    
            // 比较时刻, 任务时间相对当前时间:
            let earlier;
            if(task_hours > now_hours){
                // 在一天中靠后
                time += (task_hours - now_hours)*60 + task_minutes - now_minutes;
                earlier = false;
            }
            else if(task_hours == now_hours){
                if(task_minutes >= now_minutes){
                    // 在一天中靠后
                    time += task_minutes - now_minutes;
                    earlier = false;
                }
                else{
                    // 在一天中靠前
                    time += 24*60 + task_minutes - now_minutes;
                    earlier = true;
                }
            }
            else{
                // 在一天中靠前
                time += 24*60 + (task_hours - now_hours)*60 + task_minutes - now_minutes;
                earlier = true;
            }

            // var executeTime = new Date(Date.parse(new Date()) + 60000*time);
            // logger.info(`check ${executeTime.toDateString()} ${executeTime.toTimeString()}`);
            
            // 计算周, 得出时间
            if(task_day != -1){
                if(task_day > now_day){
                    if(earlier){
                        time += 24*60*(task_day - now_day - 1);
                    }
                    else{
                        time += 24*60*(task_day - now_day);
                    }
                }
                else if(task_day == now_day){
                    if(earlier){
                        time += 24*60*6;
                    }
                }
                else{
                    if(earlier){
                        time += 24*60*(6 - now_day + task_day);
                    }
                    else{
                        time += 24*60*(7 - now_day + task_day);
                    }
                }
            }
            // 转为秒, 若小于2分钟,则取消这个任务
            if(time <= 2){
                time = -1;
            }else{
                time = 60*time;
            }
            
        }
    
        // 与当前最近时间比较
        if(min_timeout == -1){
            min_timeout = time;
        }
        else{
            min_timeout = Math.min(time, min_timeout);
        }
    }

    // 启动计时器，因为是重启任务, 所以只需要加载时间最近的
    if(min_timeout != -1){
        restartTaskID = setTimeout(() => {
            mc.runcmd("restart");
        }, min_timeout*1000);
    }
    return min_timeout;
}

function cancelAutoRestartTask(){
    if(restartTaskID != -1){
        clearInterval(restartTaskID);
    }
}

function reloadAutoRestartTask(){
    cancelAutoRestartTask();
    AutoRestartTask = JSON.parse(file.readFrom(PATH + "/RestartTask.json"));
    return loadAutoRestartTask();
}
loadAutoRestartTask();

// 守护进程通信
var DeamonJS = {
    stopping: false,
    tick(){
        if(this.stopping) return;
        file.writeTo(PATH + "/channel.json", JSON.stringify({
            "time": Math.floor(Date.parse(new Date())/1000),
            "instruction": "tick"
        }, null , '\t'));
    },
    stop_daemon(){
        if(this.stopping) return;
        this.stopping = true;
        file.writeTo(PATH + "/channel.json", JSON.stringify({
            "time": Math.floor(Date.parse(new Date())/1000),
            "instruction": "stop"
        }, null , '\t'));
    },
    restart(){
        if(this.stopping) return;
        this.stopping = true;
        file.writeTo(PATH + "/channel.json", JSON.stringify({
            "time": Math.floor(Date.parse(new Date())/1000),
            "instruction": "restart"
        }, null , '\t'));
    }
}

// Heart beat
var scan_interval = Config["scan_interval"] * 1000;
DeamonJS.tick();
setInterval(() => {
    DeamonJS.tick();
}, scan_interval);

// Command
var voteList = {};
var waidRestartActivated = false;
var CommandManager = {
    set:function(){
        this.restart();
        this.stop();
        if(Config["vote_enable"]) this.voteRestart();
    },
    restart(){
        var cmd = mc.newCommand("restart", "Restart server", PermType.GameMasters);
        /// reload
        cmd.setEnum  ("e_reload"     , ["reload"       ]);
        cmd.mandatory("reload"       , ParamType.Enum, "e_reload"  , 1 );
        /// reload
        cmd.setEnum  ("e_wait"     , ["wait"       ]);
        cmd.mandatory("wait"       , ParamType.Enum, "e_wait"  , 1 );
        /// reload
        cmd.setEnum  ("e_cancel"     , ["cancel"       ]);
        cmd.mandatory("cancel"       , ParamType.Enum, "e_cancel"  , 1 );

        cmd.overload(["wait"]);
        cmd.overload(["wait", "cancel"]);
        cmd.overload(["reload"]);
        cmd.overload();
        
        cmd.setCallback((_cmd, _ori, out, res) => {
            if(res.wait){
                if(res.cancel){
                    waidRestartActivated = false;
                    return out.success(`Shutdown mission canceled.`);
                }
                else{
                    waidRestartActivated = true;
                    let playerList = mc.getOnlinePlayers();
                    if(playerList.length == 0){
                        mc.runcmd("restart");
                    }
                    return out.success(`The server will be shut down after all players have left.`);
                }
            }
            else if(res.reload){
                var timeout = reloadAutoRestartTask();
                if(timeout == -1){
                    return out.success(`There are no tasks to execute.`);
                }
                else{
                    var nowTime = new Date();
                    logger.info((`now: ${nowTime.toDateString()} ${nowTime.toTimeString()}`))
                    var executeTime = new Date(Date.parse(new Date()) + 1000*timeout);
                    return out.success(`The server will restart on ${executeTime.toDateString()} ${executeTime.toTimeString()}`);
                }
            }
            else{
                kickALL();
                setTimeout(() => {
                    DeamonJS.restart();
                    mc.runcmd("stop");
                }, 1000);
                return out.success("Restart server..");
            }
        });
        cmd.setup();
    },
    stop(){
        var cmd = mc.newCommand("wstop", "Kick players and stop server", PermType.GameMasters)
        cmd.overload ();
        cmd.setCallback((_cmd, _ori, out, res) => {
            kickALL();
            setTimeout(() => {
                DeamonJS.stop_daemon();
                mc.runcmd("stop");
            }, 1000);
        });
        cmd.setup();
    },
    voteRestart(){
        var cmd = mc.newCommand("voter", "Vote to restart server", PermType.Any)
        cmd.overload ();
        cmd.setCallback((_cmd, _ori, out, res) => {
            if(_ori.player){
                realName = _ori.player.realName
                // 发起投票
                if(isJsonEmpty(voteList)){
                    mc.broadcast(`[投票重启] ${realName}, 发起了重启投票。使用"/voter"跟票。`);
                    voteList[realName] = true;
                    setTimeout(() => {
                        mc.broadcast(`[投票重启] 没有足够的玩家投票，重启失败。`);
                        voteList = {};
                    }, 1000 * Config["vote_timeout"]);
                }

                // 跟票
                let playerList = mc.getOnlinePlayers();
                let voteAmount = 0;
                let totalAmount = 0;
                for (var pl of playerList) {
                    if(voteList[pl.realName] == true){
                        voteAmount ++;
                    }
                    totalAmount ++;
                }

                mc.broadcast(`[投票重启] ${realName} 投出一票。(${voteAmount}/${totalAmount})`);

                voteList[realName] = true;

                // 检查
                if(voteAmount / totalAmount >= Config["vote_percent"]){
                    mc.broadcast(`[投票重启] ${realName} 服务器将在3秒后重启...`);
                    setTimeout(() => {
                        mc.runcmd("restart")
                    }, 3000);
                }
            }
            else{
                return out.error("The origin of this command must be a player.");
            }
        });
        cmd.setup();
    },
}

// ============== MC Events ========================
mc.listen("onServerStarted", () => {
    CommandManager.set();
    // 启动守护进程
    if(Config["restart_enable"]){
        system.newProcess("AutoRestart.exe --server", () => { });
        // system.cmd("AutoRestart.exe",() => { });
        logger.info("自动重启已启用。");
    }else{
        logger.info("自动重启未启用。");
    }
});

mc.listen("onLeft", (pl) => {
    // 投票进行中
    if(!isJsonEmpty(voteList)){
        let playerList = mc.getOnlinePlayers();
        let voteAmount = 0;
        let totalAmount = 0;
        for (var pl_ of playerList) {
            if(pl_.realName != pl.realName){
                if(voteList[pl_.realName] == true){
                    voteAmount ++;
                }
                totalAmount ++;
            }
        }

        mc.broadcast(`[投票重启] ${pl.realName} 退出游戏。(${voteAmount}/${totalAmount})`);

        // 检查
        if(voteAmount / totalAmount >= Config["vote_percent"]){
            mc.broadcast(`[投票重启] ${realName} 服务器将在3秒后重启...`);
            setTimeout(() => {
                mc.runcmd("restart")
            }, 3000);
        }
    }
    // 关服任务已开启
    if(waidRestartActivated){
        let playerList = mc.getOnlinePlayers();
        if(playerList.length == 1){
            mc.runcmd("restart");
        }
    }
});
