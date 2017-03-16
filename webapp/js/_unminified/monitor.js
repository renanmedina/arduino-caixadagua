/* ------------------------------------------------------------------------------------------------------------------- */
// SystemMonitor
// @author: Renan Medina
// @date 09/03/2017 10:15:20
// @email: renan@silvamedina.com.br
// @do: Class to get and use information from arduino project that controls water pump and water tank
//      Displays logs of water pump usage and informative charts
// @uses: (Framework) jQuery
//        (Plugin) Datatables
//        (Framework) Bootstrap
/* -------------------------------------------------------------------------------------------------------------------- */

// global interval variable
var time_interval;
// SystemMonitor class definition
var SystemMonitor = {
  _currentConfig:{},
  isChartRendered:false,
  bindConfigsFormValues:function(){
    for(var c in this._currentConfig){
      var cvalue = this.getConfig(c);
      document.getElementsByName(c)[0].value = cvalue;
    }
  },
  actions:{
    SAVE_SETTINGS:"SS",
    DELETE_LOGS:"DL",
    LOAD_LOGS_TABLE: "LT"
  },
  getInfo: function(cb, cberror){
     $.ajax({
        url:this.getConfig("datasource"),
        type:"GET",
        timeout: 2000,
        success:function(result){
          var pieces = result.split(";");
          var data = {};
          for(var i in pieces){
            var spl = pieces[i].split(":");
            data[spl[0]] = spl[1];
          }
          cb(data);
        },
        error:function(e){
          cberror(e);
        },
     });
  },
  loadConfig:function(cb){
    var that = this;
    $.ajax({
      url:"configs.json",
      type:"GET",
      dateType:"json",
      success:function(confs){
        try{
          var json = JSON.parse(confs);
        }
        catch(e){
          var json = confs;
        }
        finally{
          that._currentConfig = json;
          if(json)
            cb(json);
        }
      },
    error:function(err){
      console.log(err);
    }
    });
  },
  getConfig: function(cname){
    return (this._currentConfig[cname] !== undefined ? this._currentConfig[cname] : null);
  },
  loadLogsTable:function(){
    $('#logs-table').DataTable({
      ajax:"actions.php?action="+this.actions.LOAD_LOGS_TABLE,
      searching:false,
      processing: true,
      serverSide: true,
      language: {
        url: "//cdn.datatables.net/plug-ins/9dcbecd42ad/i18n/Portuguese.json"
      }
    });
  },
  refreshLogsTable:function(){
    $("#logs-table").DataTable().draw();
  },
  clearLogs:function(){
     if(confirm("Desejar realmente excluir todos os historicos ?")){
       $.ajax({
         url:"actions.php",
         type:"POST",
         data:{action: this.actions.DELETE_LOGS},
         success:function(result){
           result = JSON.parse(result);
           if(result.num_logs){
             alert("CONCLUIDO!!! \n\n"+result.num_logs+" logs foram apagados no total.");
             SystemMonitor.refreshLogsTable();
           }
         },
         error:function(error){}
       })
     }
  },
  saveSettings:function(){
    var txt_configs = $("#settings-form").serialize();
    $.ajax({
      url:"actions.php",
      data:{action:this.actions.SAVE_SETTINGS, settings:txt_configs},
      type:"POST",
      success:function(confs){
        SystemMonitor.loadConfig(function(){
          alert("Aplicado com sucesso !");
        });
      }
    });
  },

  getInfoAndRenderCharts:function(is_first_draw){
     document.getElementById("runtime").innerHTML = new Date().toString("dd/MM/yyyy HH:mm");
     var mode_element = document.getElementById("sys-mode");
     if(!SystemMonitor.isChartRendered){
      mode_element.classList.remove("alert-danger");
      mode_element.classList.remove("alert-success");
      mode_element.classList.remove("alert-warning");
      mode_element.classList.add("alert-info");
      mode_element.innerHTML = "<i class='fa fa-refresh fa-spin'></i> carregando ...";
     }
    // get information from arduino server
    this.getInfo(function(data){
      mode_element.classList.remove("alert-danger");
      mode_element.classList.remove("alert-success");
      mode_element.classList.remove("alert-warning");
      mode_element.classList.remove("alert-info");
      var mode = data.system_mode;
      mode_element.innerHTML = (mode == 0 ? "<i class='fa fa-thumbs-down'></i> <b>DESLIGADO</b>" : mode == 1 ? "<i class='fa fa-thumbs-up'></i> <b>AUTOMATICO</b>" : "<i class='fa fa-user'></i> <b>MANUAL</b>");
      var css_class = (mode == 0 ? "alert-danger" : mode == 1 ? "alert-success" : "alert-warning");
      mode_element.classList.add(css_class);
      var gwater_json = {type:'cylinder',dataFormat:'json',renderAt:'gwater', width:"100%", dataSource:{chart:{animation:0,theme:'fint', bgColor:'#ffffff', baseFontSize:"12", showBorder:'0',cylFillColor:'#74ccf4',lowerLimit:'0', upperLimit:'100', numberSuffix:'%', showValue:'1',cylradius: '100'}, value:data.current_level}};
      var gpump_json = {type:'hlineargauge',dataFormat:'json',renderAt:'gpump',height:'65px', width:"100%", dataSource:{chart:{animation:0,theme:'fint', "showGaugeBorder": "1", gaugeBorderColor: "{light-50}",gaugeBorderThickness: "4",gaugeBorderAlpha: "100",baseFontSize:"18", baseFontColor:"#fff", bgColor:'#ffffff',showBorder:'0', showTickMarks:'0', showTickValues:'0', showValue:'0'}, colorRange:{color:[{minValue:0, maxValue:50, label:'Desligada', code:'#e74c3c'},{minValue:51, maxValue:100, label:'Ligada', code:'#2ecc71'}]}, pointers:{pointer:[{value:data.pump_state}]}}};
      if(!SystemMonitor.isChartRendered){
        $("#gwater").insertFusionCharts(gwater_json);
        $("#gpump").insertFusionCharts(gpump_json);
        SystemMonitor.isChartRendered = true;
      }
      else{
        $("#gwater").updateFusionCharts(gwater_json);
        $("#gpump").updateFusionCharts(gpump_json);
      }
    }, function(error){
      mode_element.classList.add("alert-danger");
      mode_element.innerHTML = '<i class="fa fa-unlink"></i> OFFLINE';
      $("#gwater").html("<div class='alert alert-danger'><i class='fa fa-ban'></i> Nao foi possivel carregar o nivel da caixa, favor verifique o status/conexao do <b>arduino</b>.</div>");
      $("#gpump").html("<div class='alert alert-danger'><i class='fa fa-ban'></i> Nao foi possivel carregar o estado da bomba, favor verifique o status/conexao do <b>arduino</b>.</div>");
      SystemMonitor.isChartRendered = false;
    });
  },
  init:function(){
    this.loadConfig(function(){
      time_interval = SystemMonitor.getConfig("updateInterval") * 1000;
      SystemMonitor.bindConfigsFormValues();
     
      // initialize fusion charts gadgets
      SystemMonitor.getInfoAndRenderCharts();
      SystemMonitor.loadLogsTable();
      
      var reloadMethod = function(){
        SystemMonitor.getInfoAndRenderCharts();
        // set time interval again in case user change settings
        time_interval = SystemMonitor.getConfig("updateInterval") * 1000;
        window.clearInterval(reloadIntv);
        reloadIntv = window.setInterval(reloadMethod, time_interval);
      };
      // // automatically get info and redraw charts
      var reloadIntv = window.setInterval(reloadMethod, time_interval);
    });
  }
};

 $(document).ready(function(){
   $("#tabs li a").click(function(){
     $("#tabs li").removeClass("active");
     $(this).parent().addClass("active");
     var tab_id = $(this).attr("tab-id");
     $(".tab-content").removeClass("visible");
     $(".tab-content#"+tab_id).addClass("visible");
   });
 
   // initialize system monitor
   SystemMonitor.init();
 });