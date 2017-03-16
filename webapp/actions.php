<?php
const ACTION_SAVE_SETTINGS = "SS";
const ACTION_RECEIVE_LOG = "RL";
const ACTION_LOGS_TABLE = "LT";
const ACTION_DELETE_LOGS = "DL";

$act = (isset($_POST['action']) ? $_POST['action'] : (isset($_GET['action']) ? $_GET['action'] : ''));
if(!empty($act)){
  switch($act){
    case ACTION_SAVE_SETTINGS:
      parse_str($_POST["settings"], $conf_json);
      $logs = getLogs();
      reduceLog($logs, $conf_json["logLimit"]);
      saveLogs($logs);
      file_put_contents("configs.json", json_encode($conf_json, JSON_PRETTY_PRINT));
      break;
    case ACTION_RECEIVE_LOG:
        $dt_log = date('Y-m-d H:i:s');
        $log =  $_GET["log"];
        registerLog(["date" => $dt_log,
                     "log" => $log]);                  
      break;
    case ACTION_LOGS_TABLE:
      $logs = array_reverse(getLogs());
      $confs = (array) getConfigs();
      $logs_data = [];
      $start = $_GET['start'];
      $length = $_GET['length'];
      array_walk($logs, function($log, $row_id) use(&$logs_data, $start, $length, $confs) {
        // do pager checks
        if($row_id+1 >= ($start > 1 ? $length+1 : 1) && $row_id+1 <= ($start > 1 ? $start * $length : $length))
          $logs_data[] = [date($confs["dateFormat"], strtotime($log->date)), $log->log];
      });
      $data = ["draw" => $_GET['draw']+1,
               "recordsTotal" => count($logs),
               "recordsFiltered" => count($logs),
               "data" => $logs_data];

      echo json_encode($data);
      break;
     case ACTION_DELETE_LOGS:
      // get number of logs just to use on return to client
      $logs_count = count(getLogs());
      // save logs as an empty array
      saveLogs([]);
      echo json_encode(["num_logs" => $logs_count], JSON_PRETTY_PRINT);
      break;
  }
}

function getLogs(){
  $log_json = json_decode(file_get_contents("logs.json"));
  return $log_json;
}

function registerLog($log){
  $log_json = (array) getLogs();
  $confs = (array) getConfigs();
  $limit_log = (int) $confs["logLimit"];
  // reduce log if needed
  reduceLog($log_json, $limit_log);
  if(count($log_json) == $limit_log)
    $log_json = array_slice($log_json, 1, $limit_log);
 
  $log_json[] = $log;
  saveLogs($log_json);
}

function getConfigs(){
  $conf_json = json_decode(file_get_contents("configs.json"));
  return $conf_json;
}

function reduceLog(&$logs, $limit){
  // get new size
  if(count($logs) > $limit)
    $logs = array_slice($logs, count($logs)-$limit, $limit);
}

function saveLogs($tosave){
  file_put_contents("logs.json",json_encode($tosave, JSON_PRETTY_PRINT));
}
?>