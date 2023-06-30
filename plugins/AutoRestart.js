// ==================== Title ====================

  /* ---------------------------------------- *\
   *  Name        :  AutoRestart              *
   *  Description :  自动重启                  *
   *  Version     :  0.1.9                    *
   *  Author      :  ENIAC_Jushi              *
  \* ---------------------------------------- */

// ================== Tools ==================
function isJsonEmpty(obj){
    for(var key in obj) {
        return false;
    }
    return true;
}

function kickALL(){
    var playerList = mc.getOnlinePlayers();
    for (var pl of playerList) {
        pl.kick(L("kick_message"));
    }
}
function getPlayerAmount(enableIgnore=true){
    var playerList = mc.getOnlinePlayers();
    var amount = 0;
    for(var pl in playerList){
        if(enableIgnore){
            if(!IgnoreList.inList(pl.realName)){
                amount ++;
            }
        }
        else{
            amount ++;
        }
    }
    return amount;
}

function kickAndRun(func){
    kickALL();
    setTimeout(() => {
        if(getPlayerAmount(false) == 0){
            func();
        }
        else{
            kickAndRun(func);
        }
    }, 1000);
}

function cmdOriIsGameMaster(_ori){
    if((_ori.type == 7) || (_ori.player && _ori.player.permLevel >= 1)){
        return true;
    }
    return false;
}

function isNameEqual(name1, name2){
    return name1.toLocaleLowerCase() == name2.toLocaleLowerCase();
}
// ================== Initialize ===================
let version = 0.1
const PATH = "plugins/AutoRestart";
logger.setConsole(true);
logger.setTitle('AutoRestart');
logger.info('AutoRestart is running');

//////// Config ////////
var Config;
function saveConfig(){
    file.writeTo(PATH + "/Config.json", JSON.stringify(Config , null , '\t'));
}
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
        "vote_timeout"  : 300,
        "language"      : "zh_CN"
    }
    var path = PATH + "/Config.json";
    
    if(file.exists(path)){
        Config = JSON.parse(file.readFrom(path));
        var rewrite = false;
        for(var key in defaultConfig){
            if(Config[key] == null){
                Config[key] = defaultConfig[key];
                rewrite = true;
            }
        }
        if(rewrite){
            saveConfig();
        }
    }
    else{
        Config = defaultConfig;
        saveConfig();
    }
}
loadConfig();

//////// Language ////////
var Lang = {};
function loadLanguage(type){
    var path = PATH + "/lang/" + Config["language"] +".json";
    if(file.exists(path)){
        Lang = JSON.parse(file.readFrom(path));
    }
    else{
        logger.error("Language file \"" + path + "\" not found, please check your language folder.");
    }
}
function L(format_string, ...args){
    var result = Lang[format_string];
    for(var i = 0; i < args.length; i++){
        result = result.replaceAll("{" + (i+1).toString() + "}", args[i]);
    }
    return result;
}
loadLanguage();

//////// Restart Task ////////
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

//////// Deamon ////////
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
    },
    startDaemon(){
        this.stopping = false;
        this.tick();
        startHeartBeat();
        system.newProcess("AutoRestart.exe --server", () => { });
    }
}
// Heart beat
var scan_interval = Config["scan_interval"] * 1000;
DeamonJS.tick();
var heartId = -1;
function stopHeartBeat(){
    if(heartId !=- 1){
        clearInterval(restartTaskID);
    }
}
function startHeartBeat(){
    stopHeartBeat();
    heartId = setInterval(() => {
        DeamonJS.tick();
    }, scan_interval);
}
startHeartBeat();
//////// Vote ////////
var voteList = {};
var VoteHelper = {
    timeoutID: -1,
    playerLeft(pl){
        if(this.isVoting()){
            // 计算除去该玩家的玩家总数和投票总数
            let playerList = mc.getOnlinePlayers();
            let voteAmount = 0;
            let totalAmount = 0;
            for (var pl_ of playerList) {
                if(!IgnoreList.inList(pl_.realName)){
                    if(pl_.realName != pl.realName){
                        if(voteList[pl_.realName] == true){
                            voteAmount ++;
                        }
                        totalAmount ++;
                    }
                }
            }
    
            mc.broadcast(L("voter.player_quit", pl.realName, voteAmount, totalAmount));
            
            // 检查
            if(voteAmount / totalAmount >= Config["vote_percent"]){
                mc.broadcast(L("info.voter.success"));
                setTimeout(() => {
                    mc.runcmd("restart")
                }, 3000);
            }
        }
    },
    isVoting(){
        return !isJsonEmpty(voteList);
    },
    vote(player){
        var realName = player.realName
        // 发起
        if(isJsonEmpty(voteList)){
            mc.broadcast(L("voter.start", realName));
            voteList[realName] = true;
            this.timeoutID = setTimeout(() => {
                this.cancel();
                mc.broadcast(L("voter.failed"));
            }, 1000 * Config["vote_timeout"]);
            // 检查
            this.check();
            return true;
        }

        // 跟票
        if(!voteList[realName]){
            voteList[realName] = true;
            var amounts = this.check();
            mc.broadcast(L("voter.vote", realName, amounts[0], amounts[1]));
            return true;
        }
        return false;    
    },
    check(){
        let playerList = mc.getOnlinePlayers();
        let voteAmount = 0;
        let totalAmount = 0;
        for (var pl of playerList) {
            if(!IgnoreList.inList(pl.realName)){
                if(voteList[pl.realName] == true){
                    voteAmount ++;
                }
                totalAmount ++;
            }
        }
        if(voteAmount / totalAmount >= Config["vote_percent"]){
            mc.broadcast(L("voter.success"));
            setTimeout(() => {
                mc.runcmd("restart")
            }, 3000);
        }
        return [voteAmount, totalAmount];
    },
    cancel(){
        clearInterval(this.timeoutID);
        voteList = {};
    }
}
//////// Ignore List ////////
var IgnoreList = {
    value: [],
    path: PATH + "/IgnoreList.json",
    load(){
        var defaultValue = [];
        if(file.exists(this.path)){
            this.value = JSON.parse(file.readFrom(this.path));
        }
        else{
            this.value = defaultValue;
            this.save();
        }
    },
    save(){
        file.writeTo(this.path, JSON.stringify(this.value , null , '\t'));
    },
    inList(targetName){
        for(var name of this.value){
            if(isNameEqual(name, targetName)){
                return true;
            }
        }
        return false;
    },
    add(name){
        this.load();
        if(this.inList(name)){
            return false;
        }
        else{
            this.value.push(name);
            this.save();
            return true;
        }
    },
    remove(targetName){
        this.load();
        var removed = 0;
        for(var i = 0; i < this.value.length; i++){
            if(isNameEqual(this.value[i], targetName)){
                this.value.splice(i, 1);
                removed ++;
                i--;
            }
        }
        if(removed > 0){
            this.save();
        }
        return removed;
    },
    toString(){
        var result = "";
        for(var name of this.value){
            result += ` ${name}\n`;
        }
        return result;
    }
}
IgnoreList.load();

//////// Commands ////////

var waitRestartActivated = false;
var CommandManager = {
    set:function(){
        this.restart();
        this.stop();
        if(Config["vote_enable"]) this.voteRestart();
    },
    restart(){
        var cmd = mc.newCommand("restart", "Restart server", PermType.GameMasters);
        /// help
        cmd.setEnum  ("e_help"     , ["help"       ]);
        cmd.mandatory("help"       , ParamType.Enum, "e_help"    , 1 );
        
        // enable
        cmd.setEnum  ("e_enable"   , ["enable"       ]);
        cmd.mandatory("enable"     , ParamType.Enum, "e_enable"  , 1 );
        // disable
        cmd.setEnum  ("e_disable"  , ["disable"       ]);
        cmd.mandatory("disable"    , ParamType.Enum, "e_disable" , 1 );
        
        /// wait
        cmd.setEnum  ("e_wait"     , ["wait"       ]);
        cmd.mandatory("wait"       , ParamType.Enum, "e_wait"    , 1 );
        /// wait cancel
        cmd.setEnum  ("e_cancel"   , ["cancel"       ]);
        cmd.mandatory("cancel"     , ParamType.Enum, "e_cancel"  , 1 );
        
        /// ignore
        cmd.setEnum  ("e_ignore"   , ["ignore"       ]);
        cmd.mandatory("ignore"     , ParamType.Enum, "e_ignore"  , 1 );
        /// ignore add name - add
        cmd.setEnum  ("e_add"      , ["add"       ]);
        cmd.mandatory("add"        , ParamType.Enum, "e_add"  , 1 );
        /// ignore remove name - remove
        cmd.setEnum  ("e_remove"     , ["remove"       ]);
        cmd.mandatory("remove"       , ParamType.Enum, "e_remove"  , 1 );
        /// ignore add name - name
        cmd.setEnum  ("e_name"     , ["name"       ]);
        cmd.mandatory("name"       , ParamType.String, "e_name"  , 1 );
        /// ignore show - show
        cmd.setEnum  ("e_show"     , ["show"       ]);
        cmd.mandatory("show"       , ParamType.Enum, "e_show"  , 1 );

        /// task
        cmd.setEnum  ("e_task"     , ["task"       ]);
        cmd.mandatory("task"       , ParamType.Enum, "e_task"  , 1 );

        /// reload
        cmd.setEnum  ("e_reload"   , ["reload"       ]);
        cmd.mandatory("reload"     , ParamType.Enum, "e_reload"  , 1 );


        cmd.overload(["help"]);
        
        cmd.overload(["enable"]);
        cmd.overload(["disable"]);

        cmd.overload(["wait"]);
        cmd.overload(["wait", "cancel"]);

        cmd.overload(["task", "reload"]);

        cmd.overload(["ignore", "remove", "name"]);
        cmd.overload(["ignore", "add", "name"]);
        cmd.overload(["ignore", "show"]);
        cmd.overload(["ignore", "reload"]);

        cmd.overload();
        
        cmd.setCallback((_cmd, _ori, out, res) => {
            if(res.help){
                return out.success(
                    "  wait - Restart after all players have exited.\n" + 
                    "  wait cancel - Cancel the \"wait\" task.\n" + 
                    "  task reload - Reload restart task(s) from file \"RestartTask.json\"\n"+
                    "  enable - Enable auto restart.\n" +
                    "  disable - Disable auto restart.\n" +
                    "  ignore show - Show ignore list.\n" +
                    "  ignore add - Add player to ignore list.\n" + 
                    "  ignore reload - reload ignore list."
                );
            }
            else if(res.wait){
                if(res.cancel){
                    if(waitRestartActivated){
                        waitRestartActivated = false;
                        
                        return out.success(L("restart.wait.cancel.success"));
                    }
                    else{
                        return out.error(L("restart.wait.cancel.failed"));
                    }
                }
                else{
                    if(Config["restart_enable"]){
                        waitRestartActivated = true;
                        if(getPlayerAmount() == 0){
                            mc.runcmd("restart");
                        }
                        return out.success(L("restart.wait.enable"));
                    }
                    else{
                        return out.error(L("restart.not_enable"));
                    }
                }
            }
            else if(res.task){
                if(res.reload){
                    var timeout = reloadAutoRestartTask();
                    if(timeout == -1){
                        return out.success(L("restart.task.reload.failed")); // restart.tack.reload.failed
                    }
                    else{
                        var nowTime = new Date();
                        logger.info((`now: ${nowTime.toDateString()} ${nowTime.toTimeString()}`))
                        var executeTime = new Date(Date.parse(new Date()) + 1000*timeout);
                        return out.success(L("restart.task.reload.success", executeTime.toDateString() + executeTime.toTimeString()));
                    }
                }
            }
            else if(res.enable){
                if(!Config["restart_enable"]){
                    // Start daemon
                    DeamonJS.startDaemon();
                    
                    // Write config
                    Config["restart_enable"] = true;
                    saveConfig();

                    // Reload task
                    reloadAutoRestartTask();

                    return out.success(L("restart.enable.success"));
                }
                else{
                    return out.error(L("restart.enable.failed"));
                }
            }
            else if(res.disable){
                if(Config["restart_enable"]){
                    // Stop daemon
                    DeamonJS.stop_daemon();
                    stopHeartBeat();

                    // Write config
                    Config["restart_enable"] = false;
                    saveConfig();

                    return out.success(L("restart.disable.success"));
                }
                else{
                    return out.error(L("restart.disable.failed"));
                }
            }
            else if(res.ignore){
                if(res.show){
                    var list = IgnoreList.toString();
                    if(list == ""){
                        return out.success(L("ignore.show.empty"));
                    }
                    else{
                        
                        return out.success(L("ignore.show.success", list));
                    }
                }
                else if(res.add){
                    if(IgnoreList.add(res.name)){
                        return out.success(L("ignore.add.success", res.name));
                    }
                    else{
                        return out.error(L("ignore.add.failed", res.name));
                    }
                }
                else if(res.remove){
                    var amount = IgnoreList.remove(res.name);
                    if(amount > 0){
                        return out.success(L("ignore.remove.success", amount));
                    }
                    else{
                        return out.success(L("ignore.remove.failed", res.name))
                    }
                }
                else if(res.reload){
                    IgnoreList.load();
                    return out.success(L("ignore.reload"));
                }
                
            }
            else{
                if(Config["restart_enable"]){
                    kickAndRun(() => {
                        DeamonJS.restart();
                        mc.runcmd("stop");
                    });
                    return out.success(L("restart.info"));
                }
                else{
                    return out.error(L("restart.not_enable"));
                }
            }
            return out.error(L("command.unknown"));
        });
        cmd.setup();
    },
    stop(){
        var cmd = mc.newCommand("wstop", "Kick players and stop server", PermType.GameMasters)
        cmd.overload ();
        cmd.setCallback((_cmd, _ori, out, res) => {
            kickAndRun(() => {
                DeamonJS.stop_daemon();
                mc.runcmd("stop");
            });
        });
        cmd.setup();
    },
    voteRestart(){
        var cmd = mc.newCommand("voter", "Vote to restart server", PermType.Any)

        cmd.setEnum  ("e_cancel"   , ["cancel"       ]);
        cmd.mandatory("cancel"     , ParamType.Enum, "e_cancel"  , 1 );

        cmd.overload ();
        cmd.overload (["cancel"]);

        cmd.setCallback((_cmd, _ori, out, res) => {
            if(res.cancel){
                if(cmdOriIsGameMaster(_ori)){
                    if(VoteHelper.isVoting()){
                        VoteHelper.cancel();
                        mc.broadcast(L("voter.cancel.info"));
                        return out.success(L("voter.cancel.success"));
                    }
                    else{
                        return out.error(L("voter.cancel.failed"));
                    }
                }
                else{
                    return out.error(L("command.no_permission"));
                }
            }
            else{
                if(_ori.player){
                    // 投票操作
                    if(!VoteHelper.vote(_ori.player)){
                        return out.error(L("voter.vote.failed"));
                    }
                }
                else{
                    return out.error(L("command.no_player"));
                }
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
        DeamonJS.startDaemon();
        // system.cmd("AutoRestart.exe",() => { });
        logger.info(L("onstart_enabled"));
    }else{
        logger.info(L("onstart_disabled"));
    }
});

mc.listen("onLeft", (pl) => {
    // 投票进行中
    VoteHelper.playerLeft(pl);
    // 关服任务已开启
    if(waitRestartActivated){
        if(getPlayerAmount() == 0){
            mc.runcmd("restart");
        }
    }
});
