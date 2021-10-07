/********************************************************************/
/*Учёт природного газа в конфигурации с раздельными портами, которые*/
/*распределены: первый включен как RS-485 и обслуживает модули и    */
/*индикатор, второй обслуживает MVSы , третий и четвёртый           */
/*коммуникационный протокол                                         */
/*Дата последнего изменения: 08-05-2009                             */
/*25-06-09 события на изменение в таблицах Modbus или скрипта       */
/*26-06-09 в дескриптор-14 добавлены индексы имён алармов задачи    */
/*02-07-09 введена возможн перехода к ЗУ для MVS с изменением единиц*/
/*30-09-10 удалены функции Modbus, введён новый дисплей             */
/********************************************************************/
#include<stdio.h>
#include<conio.h>
#include<mem.h>
#include<7188xa.h>
#include<X607.h>
#include<math.h>
#include<stdlib.h>
#include "all_xa.c"
#include "esy.c"
#include "mmi_new.c"

#define Max_pnt            4
struct comport             Port[4];
struct mvs                 Sensor[Max_mvs];
struct modul_ai            Ain[Max_icp_ain];
struct modul_ao            Aout[Max_icp_aout];
struct modul_dio           Dio[Max_icp_dio];
struct station             Device;
struct runtime             Prt;
struct configparam         Config[Max_pnt];
struct modbus_host         Host;
struct dynparams           Basic[Max_pnt];
struct expandparams        Expand;
struct script              Script;
struct counters            Counters;
/*struct panel               Panel;*/

const unsigned char set_ta[8] = {3,5,7,10,15,20,30,50};
const unsigned char err_pnt[6] = {9,19,29,39,49,59};
const unsigned char lnt_mvs[4] = {25,105,6,6};/*две последние цифры номера мод.*/
const unsigned char name_icp[6][2] = {{49,50},{49,55},{50,50},{50,52},{54,48},{56,48}};  //nm was:{49,50} - 12 ,{49,55} - 17 ,{50,50} - 22 ,{50,52} - 24 ,{54,48} - 60 ,{56,48} - 80
const unsigned char mvs_rd[5]={3,0,68,0,7};/* команда короткое чтение */                                                                            //now:{52,52} - 44
const unsigned char mvs_rdl[5]={3,0,64,0,29};/* команда длинное чтение */
const unsigned char mvs_wr[6]={16,0,64,0,2,12};/* команда запись адреса, тега */
const unsigned char mvs_cbr[6]={16,0,91,0,3,6};/* команды калибровки */
const unsigned char mb_page[8][2]={{0,1},{1,1},{2,2},{4,2},{6,1},{7,1},
				   {8,1},{9,1}};
const unsigned char ind_rst[4]={1,2,4,6};/*час,сутки,месяц,год*/
const unsigned char  exp_size_prm[4]={1,1,7,1};
unsigned char mvs_val[4],mvs_cmd[2];/* для калибровки */
unsigned char typ_pool,ind_dsp,mmi_pool,icp_lnt,flg_month,flg_auto,flg_init_arc,
  flg_arc_h,flg_arc_d,flg_min,flg_sec,flg_arc_clr, /* флаги вычисления, записи
	в архив, расчёта за минуту, усреднения входов */
  avg_old,dlt_tm,cnt_cbr,mmi_flg_ver,icp_pool,
  //15.09.2021
  cnt_ecosonic,cnt_ecosonic_old;

unsigned char cnt_init,icp_wr[3];/*признак периодической инициализации и счётчик*/
unsigned char Err[Max_error],flg_err[Max_error],ind_err,size_max,musor;
unsigned char Max_exp_mmi,Real_exp_dyn;
unsigned long checksum;/*контрольная сумма*/
unsigned char serial_num[8];/*заводской номер*/
/*float proba;*/unsigned char proba[15];
/*тайм-ауты от MVS и ICP,ош.вычисл.*/
void SetStatusModbus (unsigned char status);
void MyTimer (void);
void ScriptArgumentSelect (double *value,unsigned char record[],
			   unsigned char direct);
void ScriptOperateExecution (unsigned char record[]);
void ClearFlashSeg (unsigned char page,unsigned pointer);
void GetDateTime (unsigned char buf_evt[]);
unsigned char SetExpandDescript (void);
void ClearArchive ();
void WriteArchive (unsigned char typ_arc);
void InitModem ();
void InitModuleStruct (unsigned char num);
float CalcCurrent (struct modul_ao aout, unsigned char num);
void InitModuleOutStruct (unsigned char num);
void RestoreBasicParameters (unsigned char k);
void RestoreExpandParameters (void);
void RestoreSetAlarmsMVS (unsigned char num_dev);
void RestoreSetAlarmsAIN (unsigned char num_dev);
void WriteAlarmsSetup (unsigned char buf_com[]);
void SetClearAlarmsMVS (struct mvs *device,unsigned char num_pnt);
unsigned char SetClearAlarmsPrmAIN (float borders[],
	    unsigned char status,float value,unsigned char num_pnt,
				 unsigned char num_prm);
void SetClearAlarmsAIN (unsigned char num_pnt,struct modul_ai *modul);
void InitializeMain (void);
void TransmitToSensor (unsigned char buf[],unsigned char count);
unsigned char SendToICP (unsigned char number);
void ReadFromICP (unsigned char number);
unsigned char SelectSensor (unsigned char tp_prm,
			    unsigned char num_prm,float *value);
void AverageBasicParam (unsigned char num,struct dynparams *bs);
void AverageMinutExpandParam (void);
void CalculateMain (unsigned char num_pnt);
void AverageExpandParams (void);
void ReadFromArchive (unsigned char bufer[]);
void ReadFromMinArch (unsigned char bufer[]);
void ReadFromEvents (unsigned char buf_com[]);
void ViewError ();
void ReadConfigModbus (unsigned char buf_com[]);
void WriteConfigModbus(unsigned char buf_com[]);
void ReinstallPort (unsigned char number);
void GetValuesScale (unsigned char num_scale,float *lo,float *hi);
void GetAllDescript (unsigned char buf[]);
unsigned char WriteConfigParam (unsigned char size,unsigned char addr,
	  unsigned char type,unsigned char buf_com[],
	  unsigned char ind,unsigned char max_param,unsigned char num);
void GetModuleAin (unsigned char buf_com[],struct modul_ai Ain);
void GetModuleAout (unsigned char buf_com[],struct modul_ao aout);
void GetModuleDio (unsigned char buf_com[],struct modul_dio discr);
void GetModuleCounters (unsigned char buf_com[]);
void ExecuteInitialize (unsigned char *flag);
void WriteCodeScript (unsigned char buf_com[]);
void ReadCodeScript (unsigned char buf_com[]);
void CommunnicLink (unsigned char buf[],unsigned char num_port);
void ConfigSetToMMI (unsigned char config[],unsigned char select[]);
void ViewParamToMMI (double *value);
void SaveParameters (unsigned char num_pnt,struct dynparams bs);
void SaveExpParams (void);
unsigned char VerifySum (unsigned char buf[],unsigned char count);

//15.09.2021
//void SendToMVS (unsigned char number);
//void ReadFromMVS (unsigned char nmb_dev);
void SendToEcoSonic (unsigned char number);
void ReadFromEcoSonic (unsigned char number, unsigned char buf[]);
void GetDataEcoSonic(unsigned char buf[]);

/*************************/
void main (void)
{
  unsigned char i,j,k,cr[10],buf_evt[72];
  double param;
  int year,month,day,hour,min,sec;
  InitLib();
  X607_Init();
  TimerOpen();
  for (i=0;i<Max_pnt;i++) RestoreBasicParameters(i);
  InitializeMain(); 
  ReinstallPort(1);
  ReinstallPort(2);
  ReinstallPort(3);
  if (Device.set_com==1) ReinstallPort(4);
  else
  {
    InstallCom_4(9600L,8,0,1);
    Port[3].ta=10;
  } 
  InitModem();

  for (;;)
  {
    if (TimerReadValue()>=50)
    {
      TimerResetValue();
      MyTimer();
      if (Script.flag == 1)
      {
	      Script.wait++;
	      if (Script.wait >= Script.delay) 
        { 
          Script.flag=Script.wait=0;
        }
      } 
      if (Host.flag == 1)
      {
	      Host.wait++;
        if (Host.wait >= Modbus.delay*20)
	      { 
          Host.flag=Host.wait=0; 
        }
      }
    }
    if (flg_modem == 1) 
    { 
      InitModem(); 
      flg_modem=0; 
    }
    if (icp_pool == 0) /* выдача запроса в модуль через первый порт */
    {
      Prt.nmb_icp++;
      if (Prt.nmb_icp > 4) Prt.nmb_icp=0;
      if (Prt.nmb_icp <4) icp_pool=SendToICP(Prt.nmb_icp);
      else if (Prt.nmb_icp==4 && Device.mmi==1)
      icp_pool=SendToMMI(Device.mmi);
    }
    if (Port[3].status==2) /*обработка коммуник.запроса и выдача ответа 4 порт*/
    { 
      Port[3].status=0;
      CommunnicLink(Port[3].buf,4); 
    }
    if (Port[2].status==2) /*обработка коммуник.запроса и выдача ответа 3 порт*/
    { 
      Port[2].status=0;
      CommunnicLink(Port[2].buf,3); 
    }
    if (flg_init > 0) ExecuteInitialize(&flg_init);
    for (i=0;i<4;i++) if (Port[i].reinst==1) ReinstallPort(i+1);
    if (flg_init_arc==1) 
    {
      InitArchive(&Device);
      flg_arc_clr=1;
      flg_init_arc=0;
    }




    if (typ_pool == 0) /* выдача запроса в EcoSonic */
    {
       Prt.nmb++;
       if (Prt.nmb > 3) Prt.nmb=0;
       //15.09.2021
       //SendToMVS(Prt.nmb);
       SendToEcoSonic(Prt.nmb);
    }

    //15.09.2021
    if (Port[1].status == 4) /* обработка ответа от EcoSonic */
    {
      Port[1].status=0;/* вычисление контрольной суммы */
      if (typ_pool < 13) //was < 5
      {
	      //i=lnt_mvs[typ_pool-1];
        //CalcCRC16(Port[1].buf,i,cr);
        CalcCRC16(Port[1].buf,Port[1].index-2,cr);
	      //if (Port[1].buf[i]==cr[1] && Port[1].buf[i+1]==cr[0])
	        //ReadFromMVS(Prt.nmb);
        if (Port[1].buf[Port[1].index-1]==cr[0] && Port[1].buf[Port[1].index-2]==cr[1])
          ReadFromEcoSonic(Prt.nmb,Port[1].buf);
      } 
      typ_pool=Port[1].index=0;
    }




    if (Port[0].status == 4) /* обработка ответа от модулей или индикатора */
    {
      Port[0].status=0;
      /* вычисление контрольной суммы */
      if (VerifySum(Port[0].buf,Port[0].index-2)==1)
      {
	      if (icp_pool < 14) ReadFromICP(Prt.nmb_icp);
        else
          //01.09.2020 YN -----\\//----
          if (Port[0].buf[0]==0x21 &&(icp_pool==16 || icp_pool==17))
	        {
            if (step==0) 
            {
              Display.evt=0;
            }
          } //was: Display.evt=0;
          else
          //------------- -----//\\-----
	          if (icp_pool == 15 || Display.suspend==1)
  	        { /*обработка ответа от MMI подключенного к СОМ1*/
	            ViewParamToMMI(&param);
	            ReadFromMMI(Port[0].buf,Port[0].index,param);
	          }
      } 
      icp_pool=Port[0].index=0;
    }
    if (flg_arc_clr == 1) 
    { 
      ClearArchive();
      flg_arc_clr=0;
    }
    if (Device.script==1 && Script.flag==0) /*обработка скрипта*/
    {
      X607_ReadFn(0xb000+Script.count*6,6,cr);
      ScriptOperateExecution(cr);
    }
    if (flg_sec == 1) /* усреднение значений для MVS и аналог. датчиков*/
    { /*периодическое раз в секунду сохранение значений даты и времени*/
      GetDate(&year,&month,&day);
      GetTime(&hour,&min,&sec);
      if (min != Prt.old_min) 
      {
        Prt.old_min=min;
        flg_min=1;
      }
      WriteNVRAM(5,sec);
      WriteNVRAM(4,min);
      WriteNVRAM(3,hour);
      WriteNVRAM(2,day);
      WriteNVRAM(1,month);
      WriteNVRAM(0,year-2000);
      if (sec >= avg_old) dlt_tm=sec-avg_old;
      else dlt_tm=sec-avg_old+60;
      if (dlt_tm >=3)
      {
	      avg_old=sec;
        for (i=0;i<Max_pnt;i++) Config[i].flag=1;
	      Prt.flg_err=1;
        AverageExpandParams();
      }
      if (hour != Prt.old_hour)
      {
	      Prt.old_hour=hour;
        if (flg_auto==0 || flg_auto==3) flg_arc_h=1;
	      if (flg_auto==1 || (flg_auto==3 && hour>3)) flg_auto=0;
	      if (flg_auto==2) flg_auto=3;
	      if (hour==Device.contrh)
	      {
	        flg_arc_d=1;
          if (Prt.old_month != month)
	        { 
            Prt.old_month=month;
            WriteNVRAM(30,month);
            flg_month=1;
          }
	      }
      }
      for (j=0;j< Max_icp_ain;j++)
        for (i=0;i< 8;i++) Ain[j].avg[i]=(Ain[j].avg[i]*Prt.cnt_avg+
			                                  Ain[j].prm[i])/(Prt.cnt_avg+1);
      Prt.cnt_avg++;flg_sec=0;
      for (i=0;i < Max_pnt;i++)
      { /*проверка и запись в журнал алармов*/
	      SetClearAlarms(&Config[i],Basic[i],i);
	      AverageBasicParam(i,&Basic[i]);
      }
      for (i=0;i < Max_mvs;i++) SetClearAlarmsMVS(&Sensor[i],i);
      for (i=0;i < Max_icp_ain;i++) SetClearAlarmsAIN(i,&Ain[i]);
      AverageMinutExpandParam();
    }
    for (i=0;i<Max_pnt;i++) if (Config[i].flag == 1)
    { 
      CalculateMain(i);
      Config[i].flag=0;
      break;
    }/*расчёт расхода*/
    if (Prt.flg_err == 1) 
    { 
      ViewError();
      Prt.flg_err=0;
      Prt.cnt_avg=0; 
    }/*визуализация ошибок*/
    if (flg_min == 1) /* вычисление расхода за минуту,инкремент и сохранение
                        счётчиков в памяти часов реального времени*/
    {
      flg_min=0;/* обновление инициализации датчиков */




      //15.09.2021
      for (i=0;i<Max_mvs;i++)
      //if (Sensor[i].evt<1 && Sensor[i].evt>3) Sensor[i].evt=1;
      //Sensor[i].evt=3;
      Sensor[i].evt=0;



      for (i=0;i<Max_icp_ain;i++) Ain[i].evt=0;
      for (i=0;i<Max_icp_aout;i++) if (Aout[i].evt<4) Aout[i].evt=0;
      for (i=0;i<Max_icp_dio;i++) Dio[i].evt=0;
      mmi_flg_ver=1;/*проверка номера страницы индикатора*/
      for (k=0;k< Max_pnt;k++)
      { /*обработка накопленного типа данных-счётчики*/
	      for (i=0;i<Max_dyn;i++) if (main_dyn[i][2] == 2)
	      {
	        Basic[k].dyn[main_dyn[i][0]+7]=Basic[k].dyn[main_dyn[i][0]+7]/60;/*за минуту*/
	        for (j=0;j<4;j++) Basic[k].dyn[main_dyn[i][0]+ind_accum[j]]=
	                              Basic[k].dyn[main_dyn[i][0]+ind_accum[j]]+
	                                Basic[k].dyn[main_dyn[i][0]+7];
	      } 
        Basic[k].cnt[2]=0;
	      for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 2)
	      {
	        Expand.dyn[exp_dyn[i][0]+7]=Expand.dyn[exp_dyn[i][0]+7]/60;
	        for (j=0;j<4;j++) Expand.dyn[exp_dyn[i][0]+ind_accum[j]]=
	                Expand.dyn[exp_dyn[i][0]+ind_accum[j]]+Expand.dyn[exp_dyn[i][0]+7];
	      } 
        Expand.cnt[2]=0;
      } 
      WriteArchive(2);/*запись в минутный архив*/
      for (k=0;k< Max_pnt;k++)
      {
	      for (i=0;i<Max_dyn;i++) if (main_dyn[i][2] == 2)
  	      Basic[k].dyn[main_dyn[i][0]+7]=0.0;SaveParameters(k,Basic[k]);
      }
      for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 2)
        Expand.dyn[exp_dyn[i][0]+7]=0.0;SaveExpParams();
      if (flg_arc_h == 1) /* выполнение записи в часовой архив */
      {
	      flg_arc_h=0;WriteArchive(0);
	      for (i=0;i< Max_pnt;i++)
	      {
	        for (j=0;j< Max_dyn;j++) if (main_dyn[j][2] ==2)
	          Basic[i].dyn[main_dyn[j][0]+1]=0.0;Basic[i].cnt[1]=0;
	      }
	      for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 2)
	        Expand.dyn[exp_dyn[i][0]+1]=0.0;
          Expand.cnt[1]=0;
      }     /* выполнение записи в суточный архив */
      if (flg_arc_d == 1)
      {
	      flg_arc_d=0;WriteArchive(1);
	      for (i=0;i< Max_pnt;i++)
	      {
	        for (j=0;j< Max_dyn;j++) if (main_dyn[j][2] ==2)
	        {
	          Basic[i].dyn[main_dyn[j][0]+3]=Basic[i].dyn[main_dyn[j][0]+2];
	          Basic[i].dyn[main_dyn[j][0]+2]=0.0;
            Basic[i].cnt[0]=0;
	        }
	      }
	      for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 2)
	      {
	        Expand.dyn[exp_dyn[i][0]+3]=Expand.dyn[exp_dyn[i][0]+2];
	        Expand.dyn[exp_dyn[i][0]+2]=0.0;
	      } 
        Expand.cnt[0]=0;
	      if (flg_month == 1)
	      {
	        flg_month=0;
	        for (i=0;i< Max_pnt;i++)
	        {
	          for (j=0;j< Max_dyn;j++) if (main_dyn[j][2] ==2)
	          {
	            Basic[i].dyn[main_dyn[j][0]+5]=Basic[i].dyn[main_dyn[j][0]+4];
	            Basic[i].dyn[main_dyn[j][0]+4]=0.0;
	          }
	        }
	        for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 2)
	        {
	          Expand.dyn[exp_dyn[i][0]+5]=Expand.dyn[exp_dyn[i][0]+4];
	          Expand.dyn[exp_dyn[i][0]+4]=0.0;
	        }
	      }
      }
      if ((flg_auto==0)&&((Device.autogo & 1)==1)&&
	            (ReadNVRAM(2) >= 25)&&(GetWeekDay()==0))
      {
	      if ((ReadNVRAM(1)==3)&&(ReadNVRAM(3)==2)&&
	                            ((Device.autogo & 2) > 0))
	      {  /*автопереход на летнее время */
	        SetTime(ReadNVRAM(3)+1,ReadNVRAM(4),ReadNVRAM(5));
          flg_auto=1;
	        EnableEEP();
          WriteEEP(7,conf_main[11][2],1);
          ProtectEEP();
	      }
	      if ((ReadNVRAM(1)==10)&&(ReadNVRAM(3)==3)&&
	                                  ((Device.autogo & 2)==0))
	      {  /*автопереход на зимнее время*/
	        SetTime(ReadNVRAM(3)-1,ReadNVRAM(4),ReadNVRAM(5));
          flg_auto=2;
	        EnableEEP();
          WriteEEP(7,conf_main[11][2],3);
          ProtectEEP();
	      }
	      if (flg_auto > 0)
	      { /*запись события об автопереходе разрешение инициализации*/
	        FormateEvent(buf_evt);buf_evt[15]=9;flg_init=8;
	        if (flg_auto == 1) buf_evt[14]=2;
          else buf_evt[14]=3;
	        WriteEvent(buf_evt,0);
	      }
      } 
    }
  }
}

/**********  установка текущего статуса   **********************/
void SetStatusModbus (unsigned char status)
{
  Host.status=(Host.status & 0x80)+status;
  X607_WriteFn(0xaa00+Host.num_pool*8+7,1,&Host.status);
}

/**********  работа с часами и чтение СОМ портов    ************/
/*void TickTimer()
{
  count_tick++;
} */

void MyTimer (void)
{
  unsigned char a; int hour, min, sec;
  //15.09.2021 for (a=0;a<Max_mvs;a++) if (Sensor[a].wait != 0) cnt_cbr++;
  GetTime(&hour,&min,&sec);
  if (sec != Prt.old_sec) 
  {
    Prt.old_sec = sec;
    flg_sec=1;
  }
  /*получение данных от модулей и индикатора через первый порт*/
  if (icp_pool != 0) while (IsCom_1())
  {
    a=ReadCom_1();
    if (icp_pool > 4)
    {
      Port[0].status=3;
      Port[0].buf[Port[0].index]=a;
      Port[0].index++;
    }
  }
  if (icp_pool != 0)
  {
    Port[0].timer++;
    if (Port[0].timer > Port[0].ta)
    {
	    Port[0].timer=Port[0].index=Port[0].status=0;
	    icp_pool=0;
      ClearCom(1);
	    if (Err[Prt.nmb_icp+4] <10) Err[Prt.nmb_icp+4]++;/*счётчики тайм-аутов*/
    }
    if (icp_pool>4 && Port[0].status==3)
      for (a=0;a<Port[0].index;a++) if (Port[0].buf[a]==Key_termin)
      {
	      Port[0].status=4;
        Port[0].timer=0;
        Port[0].index=a;
	      ClearCom_1();
        Err[Prt.nmb_icp+4]=0;
        break;
      }
    }
  /*получение данных по коммуникациям от четвёртого порта*/
  while (IsCom_4())
  {
    Port[3].status=1;
    Port[3].buf[Port[3].index]=ReadCom_4();
    Port[3].index++;
  }
  if (Port[3].status==1)
  {
    Port[3].timer++;
    if (Port[3].timer > Port[3].ta)
    {
      Port[3].timer=Port[3].index=Port[3].status=0;
      ClearCom_4();
    }
    if (Port[3].index > 7) if (Port[3].index >= Port[3].buf[6])
    { 
      Port[3].status=2;
      Port[3].timer=Port[3].index=0;
      ClearCom_4(); 
    }
  } 
   
   
   
   
  //15.09.2021
  /* получение данных от EcoSonic через второй порт*/
  if (typ_pool != 0) while (IsCom_2())
  {
    /*a=ReadCom_2();
    if (typ_pool < 5)
    {
      if (Port[1].status==0 && a>0 && a<=240) Port[1].status=3;
      if (Port[1].status == 3)
      { 
        Port[1].buf[Port[1].index]=a;
        Port[1].index++; 
      }
    }*/
    Port[1].status=1; 
    Port[1].buf[Port[1].index]=ReadCom_2();
    Port[1].index++; 
    cnt_ecosonic=Port[1].index;
  }




  //15.09.2021 
  if (typ_pool != 0)
  {
    Port[1].timer++;
    if (Port[1].timer > Port[1].ta)
    {
	    Port[1].timer=Port[1].index=Port[1].status=0;
      typ_pool=0;
      ClearCom(2);
	    if (Err[Prt.nmb] <10) Err[Prt.nmb]++;/*счётчики тайм-аутов*/
    }
    /*if (((typ_pool==1)&&(Port[1].index >= 27)) ||
	          ((typ_pool==2)&&(Port[1].index >= 107)) ||
	            (((typ_pool==3)||(typ_pool==4))&&(Port[1].index >= 8)))
    {
	    Port[1].status=4;
      Port[1].timer=0;
      ClearCom_2();
      Err[Prt.nmb]=0;
    }*/
    if (Port[1].status==1)
    {
      if (cnt_ecosonic!=cnt_ecosonic_old) cnt_ecosonic_old=cnt_ecosonic; 
      else
      {
        Port[1].status=4; 
        Port[1].timer=0; 
        ClearCom_2();
        Err[Prt.nmb]=0;
      }
    }
  } 
  
  /*получение данных по коммуникациям от третьего порта*/
  while (IsCom_3())
  {
    Port[2].status=1;
    Port[2].buf[Port[2].index]=ReadCom_3();
    Port[2].index++;
  }
  if (Port[2].status==1)
  {
    Port[2].timer++;
    if (Port[2].timer > Port[2].ta)
    {
      Port[2].timer=Port[2].index=Port[2].status=0;
      ClearCom_3();
    }
    if (Port[2].index > 7) if (Port[2].index >= Port[2].buf[6])
    { 
      Port[2].status=2;
      Port[2].timer=Port[2].index=0;
      ClearCom_3(); 
    }
  }
}

/**************** выбор аргумента интерпретатора *********************/
void ScriptArgumentSelect (double *value,unsigned char record[],
			   unsigned char direct)
{
  float musor;
  switch (record[1])
  {
    case 0:
      ConvToFloatVerify(&musor,record[2],record[3],record[4],record[5]);
      *value=musor; 
    break;
    case 1: case 2: case 3: case 4:
      if (record[2] < Max_dyn_all)
      {
        if (direct==0) *value=Basic[record[1]-1].dyn[record[2]];
        else Basic[record[1]-1].dyn[record[2]]=*value;
      } 
    break;
    case 5:
      if (direct==0) *value=Expand.dyn[record[2]];
      else Expand.dyn[record[2]]=*value;
    break;
    case 6:
      if (record[2] < Max_exp_const) *value=exp_const[record[2]];
    break;
    case 7: case 8: case 9: case 10:
      if (record[2] < 3 && direct==0)
      *value=Sensor[record[1]-7].avg[record[2]];
    break;
    case 11: case 12:
      if (record[2] < 8 && direct==0)
        *value=Ain[record[1]-11].prm[record[2]];
    break;
    case 13: case 14:
      if (record[2] < 4 && direct!=0)
        Aout[record[1]-13].prm[record[2]]=*value;
    break;
    case 15: case 16:
      if (direct==0) *value=Dio[record[1]-15].inp;
      if (direct==1) Dio[record[1]-15].out=*value;
    break;
  }
}

/********** выполнение одной операции интерпретатора *****************/
void ScriptOperateExecution (unsigned char record[])
{
  unsigned char flag,int_acc,int_arg; flag=0;
  switch (record[0])
  {
    case 0:
      Script.count=0;
    break;
    case 1:
      ScriptArgumentSelect(&Script.accum,record,0);
    break;
    case 2:
      ScriptArgumentSelect(&Script.accum,record,1);
    break;
    case 3:
      ScriptArgumentSelect(&Script.argum,record,0);
	    Script.accum=Script.accum+Script.argum;
    break;
    case 4:
      ScriptArgumentSelect(&Script.argum,record,0);
	    Script.accum=Script.accum-Script.argum;
    break;
    case 5:
      ScriptArgumentSelect(&Script.argum,record,0);
	    Script.accum=Script.accum*Script.argum;
    break;
    case 6:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.argum != 0)
	      Script.accum=Script.accum/Script.argum;
    break;
    case 7:
      ScriptArgumentSelect(&Script.argum,record,0);
	    int_acc=Script.accum;int_arg=Script.argum;
	    Script.accum=int_acc & int_arg;
    break;
    case 8:
      ScriptArgumentSelect(&Script.argum,record,0);
	    int_acc=Script.accum;
      int_arg=Script.argum;
	    Script.accum=int_acc | int_arg;
    break;
    case 9:
      ScriptArgumentSelect(&Script.argum,record,0);
	    int_acc=Script.accum;
      int_arg=Script.argum;
	    Script.accum=int_acc ^ int_arg;
    break;
    case 10:
      ScriptArgumentSelect(&Script.argum,record,0);
	    Script.accum=fabs(Script.argum);
    break;
    case 11:
      ScriptArgumentSelect(&Script.argum,record,0);
	    Script.accum=exp(Script.argum);
    break;
    case 12:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.argum > 0)
	      Script.accum=log(Script.argum);
    break;
    case 13:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.argum > 0)
	      Script.accum=log10(Script.argum);
    break;
    case 14:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.argum >= 0)
	      Script.accum=sqrt(Script.argum);
    break;
    case 15:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum >= 0)
	      Script.accum=pow(Script.accum,Script.argum);
    break;
    case 16:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum == Script.argum) flag=1;
    break;
    case 17:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum != Script.argum) flag=1;
    break;
    case 18:  flag=1; break;
    case 19:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum > Script.argum) flag=1; 
    break;
    case 20:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum >= Script.argum) flag=1; 
    break;
    case 21:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum < Script.argum)  flag=1; 
    break;
    case 22:
      ScriptArgumentSelect(&Script.argum,record,0);
	    if (Script.accum <= Script.argum) flag=1; 
    break;
    case 23:  Script.delay=20; Script.flag=1; break;
    case 24:  Script.delay=100; Script.flag=1; break;
    case 25:  Script.delay=400; Script.flag=1; break;
  } 
  if (record[0] > 0) Script.count++;
  if (flag==1) Script.count=record[4]*256+record[5];
  if (Script.count >= 680) Script.count=0;
}

/******* сохранение указателя и очистка сегмента флэш-памяти *********/
void ClearFlashSeg (unsigned char page,unsigned pointer)
{ /*применение: инициализация, запись в архив*/
  unsigned char i,j,num_page;
  unsigned max;
  max=65535/Size_str;
  if ((pointer+1) > max)
  {
    switch (page)
    {
      case 0:
        num_page=1;
        FlashErase(0xe000);
        WriteNVRAM(15,0);
	      WriteNVRAM(16,0);
      break;
      case 1:
        num_page=2;
        FlashErase(0xc000);
        WriteNVRAM(17,0);
	      WriteNVRAM(18,0);
      break;
      case 2:
        num_page=0;
        FlashErase(0xd000);
        WriteNVRAM(13,0);
	      WriteNVRAM(14,0);
      break;
    } 
    WriteNVRAM(12,num_page);/*очищаем страницу и сохраняем номер страницы*/
  } 
  else
  {
    i=pointer/256;
    j=pointer-i*256;
    switch (page)
    {
      case 0:WriteNVRAM(13,i);WriteNVRAM(14,j);break;
      case 1:WriteNVRAM(15,i);WriteNVRAM(16,j);break;
      case 2:WriteNVRAM(17,i);WriteNVRAM(18,j);break;
    } /*сохраняем указатель*/
  }
}

/***** запись событий второго типа:отключение питания *********************/
void GetDateTime (unsigned char buf_evt[])
{
  int year,month,day,hour,min,sec;
  GetDate(&year,&month,&day);
  buf_evt[0]=year-2000;
  buf_evt[1]=month;
  buf_evt[2]=day;
  GetTime(&hour,&min,&sec);
  buf_evt[3]=hour;
  buf_evt[4]=min;
  buf_evt[5]=sec;
}

/************ разметка дескрипторов точки расширения *****************/
unsigned char SetExpandDescript (void)
{
  const unsigned char size_exp[7]={0,1,8,4,5,10,20};
  unsigned char i,j,k,n,flag;
  unsigned count;
  count=1;
  j=n=flag=0;
  for (i=0;i<Max_exp_prm;i++) if (conf_exp[i][3]==4)
  {
    ConvToFloatVerify(&exp_const[n],ReadEEP(4,conf_exp[i][2]),
	    ReadEEP(4,conf_exp[i][2]+1),ReadEEP(4,conf_exp[i][2]+2),
	      ReadEEP(4,conf_exp[i][2]+3));
    n++;
  }
  for (i=0;i<Max_exp_prm;i++) if (conf_exp[i][3]==14)
  {
    k=ReadEEP(4,conf_exp[i][2]+1);
    exp_dyn[j][0]=count-1;
    exp_dyn[j][1]=size_exp[k];
    exp_dyn[j][2]=k;
    if (k>=1 && k<7)
    {
      exp_dyn[j][3]=ReadEEP(4,conf_exp[i][2]);/*имя параметра*/
      exp_dyn[j][4]=ReadEEP(4,conf_exp[i][2]+2);/*Modbus*/
    } 
    else 
    {
      exp_dyn[j][3]=exp_dyn[j][4]=0;
    }
    count=count+exp_dyn[j][1];
    if (count>256) break;j++;
  } 
  flag=j;
  count=0;
  for (i=0;i<flag;i++) if (exp_dyn[i][2]>0 && exp_dyn[i][2]<4)
                              count=count+exp_size_prm[exp_dyn[i][2]];
  Max_exp_mmi=count;
  return flag;
}

/******** очистка час-сут архива после изм.настроек точек ************/
void ClearArchive ()
{
  unsigned char i;
  for (i=12;i<19;i++) WriteNVRAM(i,0);
  FlashErase(0xd000);
  Size_str=InitArchive(&Device);
}

/**  запись строки данных в архив для газа:0-час,1-сут,2-мин,3-польз ***/
void WriteArchive (unsigned char typ_arc)
{
  const unsigned char a[4]={1,0,2,3};
  unsigned char i,pnt_min,num_page,typ,buf_arc[Max_arch_record];
  int pnt_arc;
  unsigned segf,adrf;typ=a[typ_arc];
  i=FormateArchive(Device,Basic,typ,buf_arc,Expand);
  if (i > 0) /*число активных архивных точек*/
  {
    if (typ_arc != 2)
    {
      buf_arc[0]=typ_arc;
      for (i=0;i<6;i++) buf_arc[i+1]=ReadNVRAM(i);
      num_page=GetArcPoint(&pnt_arc,&segf);
      adrf=pnt_arc*Size_str;
      for (i=0;i<Size_str;i++) FlashWrite(segf,adrf+i,buf_arc[i]);
      pnt_arc++;
      ClearFlashSeg(num_page,pnt_arc);
    } 
    else
    {
      pnt_min=ReadNVRAM(21);
      if (pnt_min < 60)
      {
	      for (i=0;i<6;i++) buf_arc[i+1]=ReadNVRAM(i);
	      X607_WriteFn(0x6000+(pnt_min*Size_str),Size_str,buf_arc);
      } 
      pnt_min++;
      if (pnt_min >= 60) pnt_min=0;
      WriteNVRAM(21,pnt_min);
    }
  }
}

/***** инициализация модема на третьем и четвёртом порте ********/
void InitModem ()
{
  unsigned char buf_out[3];
  buf_out[0]=43;
  buf_out[1]=43;
  buf_out[2]=43; 
  ToComBufn_4(buf_out,3);
  ToComBufn_3(buf_out,3); 
  Delay(1000); 
  buf_out[0]=65; 
  buf_out[1]=84;
  buf_out[2]=13; 
  ToComBufn_4(buf_out,3); 
  ToComBufn_3(buf_out,3);
}

/*инициализация параметров структуры: модуль аналоговых входов*/
void InitModuleStruct (unsigned char num)
{
  unsigned char i,j;
  for (i=0;i<8;i++)
  {
    j=i*4+num*32;
    ConvToFloatVerify(&Ain[num].lo_brd[i],ReadEEP(6,16+j),
	              ReadEEP(6,17+j),ReadEEP(6,18+j),ReadEEP(6,19+j));
    ConvToFloatVerify(&Ain[num].hi_brd[i],ReadEEP(6,80+j),ReadEEP(6,81+j),
	              ReadEEP(6,82+j),ReadEEP(6,83+j));
    Ain[num].units[i]=ReadEEP(6,i+num*8);
  }
}

/******** вычисление значения тока для аналогового выхода *******/
float CalcCurrent (struct modul_ao aout, unsigned char num)
{
  float value;
  if ((num<4)&&(aout.hi_brd[num]>aout.lo_brd[num])&&
                    ((aout.hi_brd[num]-aout.lo_brd[num])!=0))
    value=(aout.prm[num]-aout.lo_brd[num])/
	                  ((aout.hi_brd[num]-aout.lo_brd[num])/16)+4.0;
  else
  value=0.0;
  return value;
}

/*инициализация параметров структуры: модуль аналоговых выходов*/
void InitModuleOutStruct (unsigned char num)
{
  unsigned char i,j;
  for (i=0;i<4;i++)
  {
    j=i*4+num*16;
    ConvToFloatVerify(&Aout[num].lo_brd[i],ReadEEP(6,152+j),
	                      ReadEEP(6,153+j),ReadEEP(6,154+j),ReadEEP(6,155+j));
    ConvToFloatVerify(&Aout[num].hi_brd[i],ReadEEP(6,184+j),ReadEEP(6,185+j),
	                      ReadEEP(6,186+j),ReadEEP(6,187+j));
    Aout[num].units[i]=ReadEEP(6,i+num*4+144);
  }
  for (i=0;i<2;i++)        /*коэффициенты для импульсных входов*/
  ConvToFloatVerify(&Counters.factor[i],ReadEEP(6,216+i*4),
	                  ReadEEP(6,217+i*4),ReadEEP(6,218+i*4),ReadEEP(6,219+i*4));
}

/***** восстановление базовых параметров и счётчиков *********/
void RestoreBasicParameters (unsigned char k)
{
  unsigned char i,j,ind,buf[Max_save];
  ind=0;
  X607_ReadFn(0x9000+k*Max_save,Max_save,buf);
  for (i=0;i<Max_dyn;i++) if (main_dyn[i][2]>0)
  {
    for (j=0;j<main_dyn[i][1];j++)
    {
      ConvToFloatVerify(&Basic[k].dyn[main_dyn[i][0]+j],buf[ind*4],
	                                buf[1+ind*4],buf[2+ind*4],buf[3+ind*4]);
      ind++;
    } 
    Basic[k].dyn[main_dyn[i][0]]=0.0;
    if (main_dyn[i][2]==2)
    { /*восстановление минутных приращений*/
      for (j=0;j<4;j++) Basic[k].dyn[main_dyn[i][0]+ind_rst[j]]=
	        Basic[k].dyn[main_dyn[i][0]+ind_rst[j]]+Basic[k].dyn[main_dyn[i][0]+7];
      Basic[k].dyn[main_dyn[i][0]+7]=0.0;
    }
  }
}

/*********** восстановление доп параметров и счётчиков *********/
void RestoreExpandParameters (void)
{
  unsigned char i,j,buf[4],ind;
  ind=0;
  for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2]>0 && exp_dyn[i][2]<4)
  {
    for (j=0;j<exp_dyn[i][1];j++)
    {
      X607_ReadFn(0x9000+4*(Max_save+ind),4,buf);
      ConvToFloatVerify(&Expand.dyn[exp_dyn[i][0]+j],
			                            buf[0],buf[1],buf[2],buf[3]);ind++;
    }
    if (exp_dyn[i][2]==2)
    { /*восстановление минутных приращений*/
      Expand.dyn[exp_dyn[i][0]]=0.0;
      for (j=0;j<4;j++) Expand.dyn[exp_dyn[i][0]+ind_rst[j]]=
	      Expand.dyn[exp_dyn[i][0]+ind_rst[j]]+Expand.dyn[exp_dyn[i][0]+7];
      Expand.dyn[exp_dyn[i][0]+7]=0.0;
    }
  }
}

/***** восстановление уставок алармов для MVS *********/
void RestoreSetAlarmsMVS (unsigned char num_dev)
{
  unsigned char i,j,buf[40];
  X607_ReadFn(0xc000+num_dev*64,40,buf);
  for (i=0;i<3;i++) for (j=0;j<3;j++)
    ConvToFloatVerify(&Sensor[num_dev].alm_set[i][j],buf[j*4+i*12],
                      buf[1+j*4+i*12],buf[2+j*4+i*12],buf[3+j*4+i*12]);
  Sensor[num_dev].alm_enb=buf[36];
  X607_ReadFn(0xc330+num_dev*4,4,buf);
  for (i=0;i<3;i++) Sensor[num_dev].alm_status[i]=buf[i];
}

/***** восстановление уставок алармов для аналог входов *********/
void RestoreSetAlarmsAIN (unsigned char num_dev)
{
  unsigned char i,j,buf[170];
  X607_ReadFn(0xc100+num_dev*256,170,buf);
  for (i=0;i<8;i++)
  {
    for (j=0;j<5;j++)
    ConvToFloatVerify(&Ain[num_dev].alm_set[i][j],buf[j*4+i*20],
	                  buf[1+j*4+i*20],buf[2+j*4+i*20],buf[3+j*4+i*20]);
    Ain[num_dev].alm_status[i]=buf[i+160];
  } 
  Ain[num_dev].alm_enb=buf[168];
}

/************ сохранение уставок алармов MVS или AIN в ЭНП *********/
void WriteAlarmsSetup (unsigned char buf_com[])
{
  unsigned char i,j,k,n,m,buf[4],buf_evt[16],flag,typ[6];
  unsigned offset,ofs1,ofs2; 
  n=0;
  if (buf_com[7]<4 && buf_com[8]>0 && buf_com[9]<2)
  {
    if (buf_com[9] == 0) /* для MVS*/
    {
      typ[0]=11;typ[1]=9;
      typ[2]=3;
      typ[3]=12;
      ofs1=64;
      ofs2=0;
      typ[4]=36;
      typ[5]=16;
    } 
    else
    {
      typ[0]=12;typ[1]=40;
      typ[2]=5;typ[3]=20;
      ofs1=0x100;
      ofs2=0x100;
      typ[4]=168;
      typ[5]=64;
    }
    for (i=0;i<buf_com[8];i++)
    {
      FormateEvent(buf_evt);
      flag=0;
      buf_evt[15]=typ[0];
      buf_evt[14]=buf_com[7]*typ[5]+buf_com[10+n];
      if (buf_com[10+n] < typ[1])
      {
	      j=buf_com[10+n]/typ[2];
        k=buf_com[10+n]-j*typ[2];
	      offset=ofs2+buf_com[7]*ofs1+j*typ[3]+k*4;
	      X607_ReadFn(0xc000+offset,4,buf);
	      for (m=0;m<4;m++)
	      {
	        buf_evt[6+m]=buf[m];
	        if (buf[m]!=buf_com[11+n+m]) 
          {
            buf[m]=buf_com[11+n+m];
            flag=1;
          }
	        buf_evt[10+m]=buf[m];
	      }
	      if (flag == 1)
	      { 
          X607_WriteFn(0xc000+offset,4,buf);
          WriteEvent(buf_evt,0);
        } 
        n=n+5;
      } 
      else if (buf_com[10+n] == typ[1])
      {
	      offset=ofs2+buf_com[7]*ofs1+typ[4];
	      X607_ReadFn(0xc000+offset,1,buf);
        buf_evt[6]=buf[0];
	      if (buf[0]!=buf_com[11+n]) 
        {
          buf[0]=buf_com[11+n];
          flag=1;
        }
	      buf_evt[10]=buf[0];
	      if (flag == 1)
	      { 
          X607_WriteFn(0xc000+offset,1,buf);
          WriteEvent(buf_evt,0);
        } 
        n=n+2;
      }
    } 
    if (buf_com[9]==0) RestoreSetAlarmsMVS(buf_com[7]);
    else RestoreSetAlarmsAIN(buf_com[7]);
  } 
  buf_com[6]=11;
}

/******** установка или очистка алармов MVS ***************/
void SetClearAlarmsMVS (struct mvs *device,unsigned char num_pnt)
{
  unsigned char i;
  for (i=0;i<3;i++) if ((device->alm_enb & etl[i])>0)
    device->alm_status[i]=SetClearAlarmsPrm(device->alm_set[i],
	    device->alm_status[i],device->avg[i],num_pnt,i);
}

/********* установка или очистка алармов параметра для AIN ********/
unsigned char SetClearAlarmsPrmAIN (float borders[],
	    unsigned char status,float value,unsigned char num_pnt,
				 unsigned char num_prm)
{
  /*начальный адрес статусов алармов в ЭНП:0С1A0-первый,0C2A0-второй */
  unsigned char flag,flg,cr[4],j,buf_alm[16];
  if (borders[4]>0.0) for (j=0;j<3;j++) if (borders[j+1]>borders[j]) flag=0;
  else 
  {
    flag=1;
    break;
  }
  if (flag == 0)
  {
    FormateEvent(buf_alm);
    buf_alm[13]=num_pnt*8+num_prm+8;flg=0;
    if (status == 0  && value > borders[2])
    {
      status=3;
      flg=1;
      buf_alm[10]=1;
      buf_alm[14]=1;
    } 
    else if (status == 3 && value<=(borders[2]-borders[4]))
    {
      status=0;
      flg=1;
      buf_alm[10]=0;
      buf_alm[14]=1;
    } 
    else if (status == 0 && value < borders[1])
    {
      status=2;
      flg=1;
      buf_alm[10]=1;
      buf_alm[14]=0;
    } 
    else if (status == 2 && value>= (borders[1]+borders[4]))
    {
      status=0;
      flg=1;
      buf_alm[10]=0;
      buf_alm[14]=0;
    } 
    else if (status == 3 && value > borders[3])
    {
      status=4;
      flg=1;
      buf_alm[10]=1;
      buf_alm[14]=3;
    } 
    else if (status == 4 && value<=(borders[3]-borders[4]))
    {
      status=3;
      flg=1;
      buf_alm[10]=0;
      buf_alm[14]=3;
    } 
    else if (status == 2 && value < borders[0])
    {
      status=1;
      flg=1;
      buf_alm[10]=1;
      buf_alm[14]=2;
    } 
    else if (status == 1 && value>=(borders[0]+borders[4]))
    {
      status=2;
      flg=1;
      buf_alm[10]=0;
      buf_alm[14]=2;
    }
    if (flg == 1)
    {
      ConvToBynare(value,cr);
      for (j=0;j<4;j++) buf_alm[6+j]=cr[j];
      WriteEvent(buf_alm,1);
      cr[0]=status;
      X607_WriteFn(0xc1a0+num_pnt*0x100+num_prm,1,cr);
    }
  } 
  return status;
}

/****** установка-очистка алармов одного модуля аналог вход *********/
void SetClearAlarmsAIN (unsigned char num_pnt,struct modul_ai *modul)
{
  unsigned char i;
  for (i=0;i<8;i++) if ((modul->alm_enb & etl[i])>0)
    modul->alm_status[i]=SetClearAlarmsPrmAIN(modul->alm_set[i],
	    modul->alm_status[i],modul->prm[i],num_pnt,i);
}

/* инициализация, запись:события откл.пит,час.архив,восст.счётчиков*/
void InitializeMain (void)
{
  const unsigned segm[2] = {0x8000,0x9000};
  unsigned i,j,segf,adrf;
  float val_arc;
  int pnt_arc,year,month,day,hour,min,sec;
  unsigned char tp_arc,k,buf_evt[16],cr[4];
  GetSerialNumber(serial_num);
  for (i=0;i<Max_icp_ain;i++)
  {
    InitModuleStruct(i);
    Ain[i].evt=0;
    RestoreSetAlarmsAIN(i);
  }
  for (i=0;i<Max_icp_aout;i++) 
  {
    InitModuleOutStruct(i);
    Aout[i].evt=0;
  }
  flg_arc_clr=0;
  Size_str=InitArchive(&Device);
  for (i=0;i<4;i++)
  { 
    Port[i].status=Port[i].timer=Port[i].index=Port[i].reinst=0;
  }




  //15.09.2021
  for (i=0;i<Max_mvs;i++)
  {
    Sensor[i].wait=0;
    //Sensor[i].evt=1;
    RestoreSetAlarmsMVS(i); 
    //Sensor[i].evt=3;
    Sensor[i].evt=0;
  }




  flg_init=flg_month=flg_arc_d=ind_dsp=cnt_cbr=flg_init_arc=0;
  typ_pool=Prt.cnt_avg=flg_min=flg_arc_h=0;
  InitStationStruct(&Device);
  icp_pool=Display.flag=0;
  SetDisplayPage(Typ_task);
  for (j=0;j<2;j++) for (i=0;i<65535;i++)
    checksum=checksum+FlashRead(segm[j],i);
  if (Device.task != Typ_task)
  { /*начальная инициализация:запись типа точки учёта:0-газ,1-тепло,
     2-уровень;очистка событий и архива*/
    EnableEEP();
    for (i=0;i<8;i++) for (j=0;j<256;j++) WriteEEP(i,j,0);
    WriteEEP(7,0,Typ_task);
    WriteEEP(7,1,1);
    ProtectEEP();
    for (i=6;i<22;i++) WriteNVRAM(i,0);
    FlashErase(0xd000);
    for (i=0;i<16;i++) buf_evt[i]=0;
    for (i=0;i<8192;i++) X607_WriteFn(0x0000+i*16,16,buf_evt);
  }/*запись события отключения питания*/
  for (i=0;i<Max_pnt;i++)
  {
    InitBasicStruct(i,&Config[i],&Basic[i]);
    X607_ReadFn(0x0c320+i*4,4,Config[i].status_alm);
  }
  Prt.old_month=ReadNVRAM(30);
  Real_exp_dyn=SetExpandDescript();
  RestoreExpandParameters();
  GetDateTime(buf_evt);
  for (i=0;i<6;i++) buf_evt[i+6]=ReadNVRAM(i);
  for (i=0;i<3;i++) buf_evt[i+12]=0;
  buf_evt[15]=9;
  WriteEvent(buf_evt,0);
  GetDate(&year,&month,&day);
  GetTime(&hour,&min,&sec);
  if ((ReadNVRAM(2)==day && ReadNVRAM(3)>=Device.contrh)||
    (ReadNVRAM(2)==day && ReadNVRAM(3)<Device.contrh && hour<Device.contrh)||
      (ReadNVRAM(2)!=day && ReadNVRAM(3)>=Device.contrh && hour<Device.contrh))
        /*выбор типа архива:часовой или суточный*/
          tp_arc=0;
  else tp_arc=1;
  WriteArchive(tp_arc);
  Prt.old_min=min;
  Prt.old_hour=hour;
  for (k=0;k<Max_pnt;k++) /*обнуление счётчиков*/
  {
    for (i=0;i< Max_dyn;i++) if (main_dyn[i][2] ==2)
    {
      Basic[k].dyn[main_dyn[i][0]+1]=0.0;
      if (tp_arc==1)
      {
	      Basic[k].dyn[main_dyn[i][0]+3]=Basic[k].dyn[main_dyn[i][0]+2];
	      Basic[k].dyn[main_dyn[i][0]+2]=0.0;
      }
    }
  }
  for (i=0;i< Real_exp_dyn;i++) if (exp_dyn[i][2] ==2)
  {
    Expand.dyn[exp_dyn[i][0]+1]=0.0;
    if (tp_arc==1)
    {
      Expand.dyn[exp_dyn[i][0]+3]=Expand.dyn[exp_dyn[i][0]+2];
      Expand.dyn[exp_dyn[i][0]+2]=0.0;
    }
  }
}

/********* передача запроса в порт COM2 *************************/
void TransmitToSensor (unsigned char buf[],unsigned char count)
{
  ToComBufn_2(buf,count);/*WaitTransmitOver_2();*/
}

//15.09.2021
/********** формирование запроса в EcoSonic **************************/
void SendToEcoSonic (unsigned char number)
{
  unsigned char i, bufs[30],  cr[2], count;
  if (Device.adr_mvs[number]>0)
  {
    bufs[0]=Device.adr_mvs[number];
    switch (Sensor[number].evt)
    {
      case 0: /*05e4-1508-Flow Rate m3/h переменные*/
        bufs[1]=3; bufs[2]=5; bufs[3]=228; bufs[4]=0; bufs[5]=2; 
        count=6; typ_pool=1;
      break;

      case 1: /*05ee-1518-Pressure Bar переменные*/
        bufs[1]=3; bufs[2]=5; bufs[3]=238; bufs[4]=0; bufs[5]=2; 
        count=6; typ_pool=2;
      break;
      
      case 2: /*05ec-1516-Temperature C переменные*/
        bufs[1]=3; bufs[2]=5; bufs[3]=236; bufs[4]=0; bufs[5]=2; 
        count=6; typ_pool=3;
      break;
    } 
    CalcCRC16(bufs,count,cr); 
    bufs[count]=cr[1]; 
    bufs[count+1]=cr[0];
    count=count+2; 
    TransmitToSensor(bufs,count);
    cnt_ecosonic=0; 
    cnt_ecosonic_old=0;
  } 
  else Err[Prt.nmb]=0;
}

//15.09.2021
void ReadFromEcoSonic (unsigned char number, unsigned char buf[])
{
  unsigned char i, num;  num=3;
  switch (typ_pool)
  {
    case 1:
      ConvToFloatVerify (&Sensor[number].data[1],  buf[num+3], buf[num+2], buf[num+1], buf[num]);
	    Sensor[number].avg[0]=Sensor[number].data[1];
	    Sensor[number].evt=1;
    break;
    case 2:
      ConvToFloatVerify (&Sensor[number].data[2], buf[num+3], buf[num+2], buf[num+1], buf[num]);
      Sensor[number].data[2] *= 100; // датчик отдает в Бар, а надо КПа -> 1Бар=100КПа
	    Sensor[number].avg[1]=Sensor[number].data[2];
	    Sensor[number].evt=2;
    break;
    case 3:
      ConvToFloatVerify (&Sensor[number].data[3], buf[num+3], buf[num+2], buf[num+1], buf[num]);
	    Sensor[number].avg[2]=Sensor[number].data[3];
	    Sensor[number].evt=0;
    break;
  }
}

//15.09.2021
/************* получение данных от датчика **********************/
void GetDataEcoSonic(unsigned char buf_com[])
{
  unsigned char i, j, k, cr[4];
  if (buf_com[7]<4)
  {
    for (i=0;i<3;i++)
    {
      ConvToBynare(Sensor[buf_com[7]].data[i+1],cr);
      for (k=0;k< 4;k++) buf_com[8+i*4+k]=cr[k];
    }
    for (i=0;i<3;i++) buf_com[20+i]=Sensor[buf_com[7]].units[i];
    for (i=0;i<6;i++)
    {
      ConvToBynare(Sensor[buf_com[7]].data[i+4],cr);
      for (k=0;k< 4;k++) buf_com[24+i*4+k]=cr[k];
    } for (i=0;i<3;i++) buf_com[48+i]=Sensor[buf_com[7]].err[i];
    buf_com[6]=56;
  }
}

/********** формирование запроса в ICP ***************************/
unsigned char SendToICP (unsigned char number)
{ /*введён первым символом: возврат каретки*/
  unsigned char cr[2],count,bufs[20],evt,out,buf[50],status,chanel,i;
  unsigned char pool;
  float value; 
  pool=0;
  if (Device.adr_icp[number]>0)
  {
    switch (Device.typ_icp[number])
    {
      case 0:evt=Ain[0].evt;status=Ain[0].status[3];break;
      case 1:evt=Ain[1].evt;status=Ain[1].status[3];break;
      case 2:
        evt=Aout[0].evt;status=Aout[0].status[3];
	      chanel=Aout[0].chnl;
        value=CalcCurrent(Aout[0],chanel);
      break;
      case 3:
        evt=Aout[1].evt;status=Aout[1].status[3];
	      chanel=Aout[1].chnl;
        value=CalcCurrent(Aout[1],chanel);
      break;
      case 4:evt=Dio[0].evt;status=Dio[0].status[3];out=Dio[0].out;break;
      case 5:evt=Dio[1].evt;status=Dio[1].status[3];out=Dio[1].out;break;
      case 6:evt=Counters.evt;status=Counters.status[3];break;
    }
    bufs[1]=hex_to_ascii[(Device.adr_icp[number] >> 4) & Key_mask];
    bufs[2]=hex_to_ascii[Device.adr_icp[number] & Key_mask];
    switch (evt)
    {
      case 0:
        bufs[0]=Key_usa;
        bufs[3]=Key_M;
        count=4;
	      pool=5;
        evt=1;
      break;/*запрос имени модуля "$xxM"*/
      case 1:
        bufs[0]=Key_usa;
        bufs[3]=Key_2;
        count=4;
	      pool=6;
        if ((status>0 && status<3) || status>3) evt=2;
        else if (status == 3) evt=8; 
        if (status == 6) evt=12;
      break;/*запрос статуса модуля "$xx2"*/
      case 2:
        if (status==1 || status==2) /*чтение 7017 #xx */
	      {
          bufs[0]=Key_dies;
          count=3;
          pool=7;
        } 
        else if (status==3 || status==4) /*установка 7024 по номеру канала*/
	      {
	        bufs[0]=Key_dies;
          pool=7;
	        bufs[3]=hex_to_ascii[chanel & Key_mask];
	        /* преобразование веществ числа в строку */
	        if (status==3) 
          {
            FloatToText(value,bufs,4);
            count=10;
          }
	        if (status==4)
	        {
            bufs[4]=0x2b;
            FloatToText(value,bufs,5);
            count=11;
          } 
          chanel++;
          /*7024 out value*/
	        if ((status==3 && chanel>1)||(status==4 && chanel>3))
		        chanel=0;
	      } 
        else if (status==5) 
        {
          bufs[0]=Key_adr;
          count=3;
          pool=7;
          evt=3;
        }/*запрос блока данных модуля 7060 di-do "@xx" */
	    break;
      case 3:
        if (status==5)
	      {  /*установка дискретных выходов 7060* @xxF !!!!!!!!!!  7044 @xxFF */
	        bufs[0]=Key_adr;
          count=4;
          evt=2; //nm was (count=4; for 7060) (count=5; for 7044)
	        bufs[3]=hex_to_ascii[(out) & Key_mask]; //nm for 7060 раскоментировать а две строчки ниже закоментировать
          //bufs[4]=hex_to_ascii[(out) & Key_mask]; //nm added this string for 7044
          //bufs[3]=0x30; //nm '0' added this string for 7044
	     } 
       else
	     {
	        bufs[0]=0x25;count=11;evt=1;/*"%xx"*/
	        bufs[3]=hex_to_ascii[(icp_wr[0] >> 4) & Key_mask];
	        bufs[4]=hex_to_ascii[icp_wr[0] & Key_mask];/*новый адрес*/
	        bufs[5]=hex_to_ascii[(icp_wr[1] >> 4) & Key_mask];
	        bufs[6]=hex_to_ascii[icp_wr[1] & Key_mask];/*новый тип входа*/
	        bufs[7]=Key_0;bufs[8]=Key_6;
	        bufs[9]=hex_to_ascii[(icp_wr[2] >> 4) & Key_mask];
	        bufs[10]=hex_to_ascii[icp_wr[2] & Key_mask];/*новый формат*/
	     } 
       pool=8;
      break;/*установка статуса*/
      case 4:
        if (status<3) /*разр-запрет калибровки "~xxE 7017"*/
	      {
	        bufs[0]=0x7e;
          bufs[3]=Key_E;
          count=5;
          pool=14;
	        bufs[4]=hex_to_ascii[icp_wr[0] & Key_mask];
          evt=2;
	      } 
        else
	      {
	        bufs[0]=Key_dies;
          pool=7;
	        bufs[3]=hex_to_ascii[icp_wr[1] & Key_mask];
	        switch (icp_wr[0])
	        {
		        case 1:
              FloatToText(4.0,bufs,4);
              count=10;
            break;
		        case 2:
              bufs[4]=0x2b;
              FloatToText(0.0,bufs,5);
              count=11;
            break;
		        case 3:
              if (status==3) 
              {
                FloatToText(20.0,bufs,4);
                count=10;
              } 
              else
			        {
                bufs[4]=0x2b;
                FloatToText(20.0,bufs,5);
                count=11;
              } 
            break;
	        } 
          evt=10;
	      } 
      break;
      case 5:
        bufs[0]=Key_usa;
        bufs[3]=Key_1;
        if (status<3) count=4;
        else
	      {
          bufs[4]=hex_to_ascii[icp_wr[1] & Key_mask];
          count=5;
        }
	      pool=14;
        evt=2;
      break;/*калибровка Zero "$xx1"*/
      case 6:
        bufs[0]=Key_usa;
        bufs[3]=Key_0;
        if (status<3)
	      {
          count=4;
          evt=2;
        } 
        else
	      {
	        bufs[4]=hex_to_ascii[icp_wr[1] & Key_mask];
          count=5;
	        evt=4;
          icp_wr[0]=3;
	      } 
        pool=14;
      break;/*калибровка Span "$xx0"*/
      case 8:
        bufs[0]=Key_usa;
        bufs[3]=Key_9;
        bufs[4]=Key_0;
        count=5;
	      pool=9;
        evt=9;
      break;/*допстатус 0-канал*/
      case 9:
        bufs[0]=Key_usa;
        bufs[3]=Key_9;
        bufs[4]=Key_1;
        count=5;
	      pool=10;
        evt=2;
      break;/*допстатус 1-канал*/
      case 10:  count=0;break;
      case 11:
        bufs[0]=Key_usa;
        bufs[3]=Key_3;
        pool=14;
        evt=10;
	      bufs[4]=hex_to_ascii[icp_wr[1] & Key_mask];/*подстройка*/
	      bufs[5]=hex_to_ascii[(icp_wr[0] >> 4) & Key_mask];
	      bufs[6]=hex_to_ascii[icp_wr[0] & Key_mask];
        count=7;
      break;
      case 12:
        bufs[0]=Key_usa;
        bufs[3]=Key_5;
        count=5;/*$xx5N читать статус счётчика*/
	      bufs[4]=hex_to_ascii[Counters.chan & 1];
        pool=11;
      break;
      case 13:
        bufs[0]=Key_dies;
        count=4;/*#xxN  читать значение счётчика*/
	      bufs[3]=hex_to_ascii[Counters.chan & 1];
        pool=12;
      break;
      case 14:
        bufs[0]=Key_usa;
        bufs[3]=Key_5;
        count=6;/*$xx5NS записать статус счётчика*/
	      bufs[4]=hex_to_ascii[Counters.chan & 1]; evt=12;
        pool=14;
	      bufs[5]=hex_to_ascii[Counters.start[Counters.chan] & 1];
	      Counters.chan=0;
      break;
      case 15:
        bufs[0]=Key_usa;
        bufs[3]=Key_6;
        count=5;/*$xx6N сбросить счётчик*/
	      bufs[4]=hex_to_ascii[icp_wr[0] & 1];
        pool=14;
        evt=13;
      break;
    }  
    if (count > 0)
    {
      CalcCheckSum(bufs,count,cr);
      bufs[count]=hex_to_ascii[cr[0]];
      bufs[count+1]=hex_to_ascii[cr[1]];
      bufs[count+2]=Key_termin;
      count=count+3;
      for (i=count;i>0;i--) bufs[i]=bufs[i-1];
      bufs[0]=Key_termin;
      count++;
      ToComBufn_1(bufs,count);
    }
    switch (Device.typ_icp[number])
    {
      case 0:Ain[0].evt=evt;break;
      case 1:Ain[1].evt=evt;break;
      case 2:Aout[0].evt=evt;Aout[0].chnl=chanel;break;
      case 3:Aout[1].evt=evt;Aout[1].chnl=chanel;break;
      case 4:Dio[0].evt=evt;break;
      case 5:Dio[1].evt=evt;break;
      case 6:Counters.evt=evt;break;
    }
  } 
  else Err[Prt.nmb_icp+4]=0; 
  return pool;
}

/******* обработка данных от ICP ****************************/
void ReadFromICP (unsigned char number)
{
  unsigned char i,j,m,buf_evt[16],cr[4],status[4],inp,stat_out;
  float buf_data[8];
  switch (Device.typ_icp[number])
  {
    case 0:status[3]=Ain[0].status[3];break;
    case 1:status[3]=Ain[1].status[3];break;
    case 2:status[3]=Aout[0].status[3];break;
    case 3:status[3]=Aout[1].status[3];break;
    case 4:status[3]=Dio[0].status[3];break;
    case 5:status[3]=Dio[1].status[3];break;
    case 6:status[3]=Counters.status[3];break;
  }
  switch (icp_pool)
  {
    case 5:
      if (Port[0].buf[0]==0x21) for (i=0;i< 6;i++)
	      if ((Port[0].buf[5]==name_icp[i][0])&&(Port[0].buf[6]==name_icp[i][1]))
	    {
        status[3]=i+1;
        break;
      }
	    switch (Device.typ_icp[number])
	    {
	      case 0:Ain[0].status[3]=status[3];break;
	      case 1:Ain[1].status[3]=status[3];break;
	      case 2:Aout[0].status[3]=status[3];break;
	      case 3:Aout[1].status[3]=status[3];break;
	      case 4:Dio[0].status[3]=status[3];break;
	      case 5:Dio[1].status[3]=status[3];break;
	      case 6:Counters.status[3]=status[3];break;
	    } 
    break;/*чтение имени модуля ICP*/
    case 6:
      if (Port[0].buf[0]==0x21)
	    {
	      status[0]=ascii_to_hex(Port[0].buf[3])*16+
				      ascii_to_hex(Port[0].buf[4]);/*тип входа*/
	      status[1]=ascii_to_hex(Port[0].buf[7])*16+
				      ascii_to_hex(Port[0].buf[8]);/*формат и фильтр*/
	       status[2]=ascii_to_hex(Port[0].buf[1])*16+
				      ascii_to_hex(Port[0].buf[2]);/*адрес модуля*/
	    }
	    switch (Device.typ_icp[number])
	    {
	      case 0:for (i=0;i<3;i++) Ain[0].status[i]=status[i];break;
	      case 1:for (i=0;i<3;i++) Ain[1].status[i]=status[i];break;
	      case 2:for (i=0;i<3;i++) Aout[0].status[i]=status[i];break;
	      case 3:for (i=0;i<3;i++) Aout[1].status[i]=status[i];break;
	      case 4:for (i=0;i<3;i++) Dio[0].status[i]=status[i];break;
	      case 5:for (i=0;i<3;i++) Dio[1].status[i]=status[i];break;
	      case 6:for (i=0;i<3;i++) Counters.status[i]=status[i];break;
	    } 
    break;/*чтение статуса модуля ICP 7060*/
    case 7:
      if (Port[0].buf[0]==0x3e)
	    {
	      if (status[3]==1 || status[3]==2)
	      { /*чтение данных модуля аналоговых входов*/
	        TextToFloat(Port[0].index,Port[0].buf,buf_data);
	        for (i=0;i<8;i++)
	        {
		        Ain[Device.typ_icp[number]].prm[i]=buf_data[i]*1.025;
		        if (Ain[Device.typ_icp[number]].hi_brd[i] >
		          Ain[Device.typ_icp[number]].lo_brd[i])
		            Ain[Device.typ_icp[number]].prm[i]=
		              (Ain[Device.typ_icp[number]].prm[i]-4.0)*
		                (Ain[Device.typ_icp[number]].hi_brd[i]-
		                  Ain[Device.typ_icp[number]].lo_brd[i])/16+
		                    Ain[Device.typ_icp[number]].lo_brd[i];
	        }
	      } 
        else if (status[3]==5)
	      {
	        inp=ascii_to_hex(Port[0].buf[3])*16+ascii_to_hex(Port[0].buf[4]);  //!!!!!!!!!!!!!!поолучение данных о входах и выходах 7060
	        stat_out=ascii_to_hex(Port[0].buf[1])*16+
				    ascii_to_hex(Port[0].buf[2]);
	        switch (Device.typ_icp[number])
	        {
		        case 4:Dio[0].inp=inp;Dio[0].stat_out=stat_out;break;
		        case 5:Dio[1].inp=inp;Dio[1].stat_out=stat_out;break;
	        }
	      }
	    } 
    break;
    case 8:
      if (status[3]!=5 && Port[0].buf[0]==0x21) /*запись события если адрес ICP изменён*/
	    {
	      m = ascii_to_hex(Port[0].buf[1])*16+ascii_to_hex(Port[0].buf[2]);
	      if (Device.adr_icp[number] != m)
	      {
		      EnableEEP();
          WriteEEP(7,8+(number),m);
          ProtectEEP();
		      FormateEvent(buf_evt);
          buf_evt[15]=8;
		      buf_evt[14]=8+(number);
          buf_evt[6]=Device.adr_icp[number];
		      buf_evt[10]=m;
          WriteEvent(buf_evt,0);
	      }
	    } 
    break;
    case 9:
      if (Port[0].buf[0]==0x21)
	    {
	      status[0]=(status[0] & 0xf0)+ascii_to_hex(Port[0].buf[3]);/*тип входа*/
	      status[0]=(status[0] & 0xf)+(ascii_to_hex(Port[0].buf[4]) << 4);
	      switch (Device.typ_icp[number])
	      {
		      case 2:Aout[0].status[4]=status[0];break;
		      case 3:Aout[1].status[4]=status[0];break;
	      }
	    } 
    break;/*чтение допстатуса модуля аналог выхода 0*/
    case 10:
      if (Port[0].buf[0]==0x21)
	    {
	      status[0]=(status[0] & 0xf0)+ascii_to_hex(Port[0].buf[3]);/*тип входа*/
	      status[0]=(status[0] & 0xf)+(ascii_to_hex(Port[0].buf[4]) << 4);
	      switch (Device.typ_icp[number])
	      {
		      case 2:Aout[0].status[5]=status[0];break;
		      case 3:Aout[1].status[5]=status[0];break;
	      }
	    } 
    break;/*чтение допстатуса модуля аналог выхода 1*/
    case 11:
      Counters.start[Counters.chan]=ascii_to_hex(Port[0].buf[3]);
	      Counters.chan++;
	      if (Counters.chan >1) 
        {
          Counters.chan=0;
          Counters.evt=13;
        }
    break;/*чтение статуса счётчика*/
    case 12:
      if (Port[0].buf[0]==0x3e) /*0x3e ">" */
	    { /*здесь должно быть вычисление зн счётчика*/
	      Counters.counter[Counters.chan]=(ascii_to_hex(Port[0].buf[1])*4096.0+
		      ascii_to_hex(Port[0].buf[2])*256.0+
		        ascii_to_hex(Port[0].buf[3])*16.0+
		          ascii_to_hex(Port[0].buf[4]))*65536.0+
		            (ascii_to_hex(Port[0].buf[5])*16.0+ascii_to_hex(Port[0].buf[6]))*256.0+
		              ascii_to_hex(Port[0].buf[7])*16.0+ascii_to_hex(Port[0].buf[8]);
	      if (Counters.factor[Counters.chan]>0) /*в технических единицах*/
	      Counters.value[Counters.chan]=Counters.counter[Counters.chan]/
					Counters.factor[Counters.chan];
	      Counters.chan++; 
        if (Counters.chan >1) Counters.chan=0;
	    } 
    break;/*чтение значения счётчика*/
  }
}

/****** подключение входа точки учёта к датчику ***************/
unsigned char SelectSensor (unsigned char tp_prm,
			    unsigned char num_prm,float *value)
{
  unsigned char flag;
  flag=0;
  switch (tp_prm)
  {
    case 1: *value=Sensor[0].avg[num_prm];break;
    case 2: *value=Sensor[1].avg[num_prm];break;
    case 3: *value=Sensor[2].avg[num_prm];break;
    case 4: *value=Sensor[3].avg[num_prm];break;
    case 5: *value=Ain[0].avg[0];break;
    case 6: *value=Ain[0].avg[1];break;
    case 7: *value=Ain[0].avg[2];break;
    case 8: *value=Ain[0].avg[3];break;
    case 9: *value=Ain[0].avg[4];break;
    case 10: *value=Ain[0].avg[5];break;
    case 11: *value=Ain[0].avg[6];break;
    case 12:*value=Ain[0].avg[7];break;
    case 13:*value=Ain[1].avg[0];break;
    case 14:*value=Ain[1].avg[1];break;
    case 15:*value=Ain[1].avg[2];break;
    case 16:*value=Ain[1].avg[3];break;
    case 17:*value=Ain[1].avg[4];break;
    case 18:*value=Ain[1].avg[5];break;
    case 19:*value=Ain[1].avg[6];break;
    case 20:*value=Ain[1].avg[7];break;
    default:flag=1;
  } 
  return flag;
}

/************** Усреднение основных переменных *****************/
void AverageBasicParam (unsigned char num,struct dynparams *bs)
{
  unsigned char i,j,cr[4],ind;
  float musor,val_diff;
  if ((Config[num].status) == 1) /*если точка учёта включена*/
  {
    if (SelectSensor (Config[num].s_diff,0,&val_diff)!=0) goto M1;/*перепад в ручном*/
    if (SelectSensor (Config[num].stack,0,&musor)==0)
    { /*дополнительный перепадчик подключен,проверка границ переключения*/
      if ((Config[num].hi_stack<=0)||(Config[num].lo_stack<=0)
	      ||(Config[num].hi_stack<=Config[num].lo_stack))
      {
	      if (Err[8+err_pnt[num]]<10)
	        Err[8+err_pnt[num]]=Err[8+err_pnt[num]]+4;goto M;
      } 
      else Err[8+err_pnt[num]]=0;
      if (Config[num].cur_sens==0)
      { /*если используется дополнительный перепадчик*/
	      if (musor>=Config[num].hi_stack)
	      {
          Config[num].cur_sens=1;
          bs->dyn[0]=val_diff;
        }
	      else bs->dyn[0]=musor;
      } 
      else
      { /*если используется основной перепадчик*/
	      if (val_diff<=Config[num].lo_stack)
	      {
          Config[num].cur_sens=0;
          bs->dyn[0]=musor;
        }
	      else bs->dyn[0]=val_diff;
      } 
      goto M1;
    }   /*подключение перепада*/
    M:  bs->dyn[0]=val_diff;/*получение значения перепада*/
    M1: if (SelectSensor (Config[num].s_press,1,&musor)==0)
	        bs->dyn[4]=musor;/*получение давления*/
        if (SelectSensor (Config[num].s_tempr,2,&musor)==0)
	      bs->dyn[8]=musor;/*получение температуры*/
        for (i=0;i<Max_dyn;i++)
        { /*все минутные усреднения и накопления*/
          if (main_dyn[i][2] == 3)
            bs->dyn[main_dyn[i][0]+3]=
              (bs->dyn[main_dyn[i][0]+3]*bs->cnt[2]+bs->dyn[main_dyn[i][0]])/
		            (bs->cnt[2]+1);
          if (main_dyn[i][2] == 2) bs->dyn[main_dyn[i][0]+7]=
            (bs->dyn[main_dyn[i][0]+7]*bs->cnt[2]+bs->dyn[main_dyn[i][0]])/
		          (bs->cnt[2]+1);
        } 
        ind=0;
        for (i=0;i<Max_dyn;i++) if (main_dyn[i][2]>0)
          for (j=0;j<main_dyn[i][1];j++)
        { /*сохранение минутных приращений*/
        if (main_dyn[i][2]==2 && j==7)
        {
	        musor=bs->dyn[main_dyn[i][0]+j]/3600*bs->cnt[2];
	        ConvToBynare(musor,cr);
	        X607_WriteFn(0x9000+num*Max_save+ind*4,4,cr);
        } 
        ind++;
      } 
      bs->cnt[2]++;
  }
}

/************** Усреднение дополнительных переменных ***********/
void AverageMinutExpandParam (void)
{
  unsigned char i,j,cr[4],ind;
  for (i=0;i<Real_exp_dyn;i++)
  { /*все минутные усреднения и накопления*/
    if (exp_dyn[i][2] == 3)
      Expand.dyn[exp_dyn[i][0]+3]=(Expand.dyn[exp_dyn[i][0]+3]*Expand.cnt[2]+
        Expand.dyn[exp_dyn[i][0]])/(Expand.cnt[2]+1);
    if (exp_dyn[i][2] == 2) Expand.dyn[exp_dyn[i][0]+7]=
      (Expand.dyn[exp_dyn[i][0]+7]*Expand.cnt[2]+Expand.dyn[exp_dyn[i][0]])/
		    (Expand.cnt[2]+1);
  } 
  ind=0;
  for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2]>0 && exp_dyn[i][2]<4)
    for (j=0;j<exp_dyn[i][1];j++)
  { /*сохранение минутных приращений*/
    if (exp_dyn[i][2]==2 && j==7)
    {
      ConvToBynare(Expand.dyn[exp_dyn[i][0]+j]/3600*Expand.cnt[2],cr);
      X607_WriteFn(0x9000+(Max_save+ind)*4,4,cr);
    } 
    ind++;
  } 
  Expand.cnt[2]++;
}

/************* выполняет основной расчёт ***********************/
void CalculateMain (unsigned char num_pnt)
{
  unsigned char i,j,res;
  if ((Config[num_pnt].status) == 1) /*если точка учёта включена*/
  {
    res=CalcFlowMain(Config[num_pnt],&Basic[num_pnt]);
    for (i=0;i<8;i++)
    { /*регистрация ошибок*/
      j=i+err_pnt[num_pnt];
      if ((res & etl[i])==0) Err[j]=0;
      else if (Err[j]<10) Err[j]=Err[j]+4;
    } /*для задачи газа коррекция по минимальному расходу*/
    #if (Typ_task==0 || Typ_task==2)
      if (res==0 && Config[num_pnt].flow_corr==1 &&
	      Basic[num_pnt].dyn[0]>=Config[num_pnt].cut_flow &&
	        Basic[num_pnt].dyn[24]<Config[num_pnt].borders[3][0])
	          Basic[num_pnt].dyn[24]=Config[num_pnt].borders[3][0];
    #endif
    #if (Typ_task==1 || Typ_task==3)
      if (Device.units == 1)
          Basic[num_pnt].dyn[40]=Basic[num_pnt].dyn[40]/4.1868;
      Basic[num_pnt].dyn[32]=Basic[num_pnt].dyn[16]*
			  Basic[num_pnt].dyn[40]/1E3;
    #endif
    /*должно быть присвоение мгн.значений*/
    for (i=0;i<Max_dyn;i++) if (main_dyn[i][2] == 3) for (j=0;j<2;j++)
      Basic[num_pnt].dyn[main_dyn[i][0]+j+1]=
        (Basic[num_pnt].dyn[main_dyn[i][0]+j+1]*
          Basic[num_pnt].cnt[j]+Basic[num_pnt].dyn[main_dyn[i][0]])/
            (Basic[num_pnt].cnt[j]+1);
    for (j=0;j<2;j++) Basic[num_pnt].cnt[j]++;
  } 
  else
  {
    #if (Typ_task==1 || Typ_task==3)
      Basic[num_pnt].dyn[16]=0.0;
    #endif
    Basic[num_pnt].dyn[24]=0.0;
    for (i=0;i<8;i++) 
    {
      j=i+err_pnt[num_pnt];
      Err[j]=0;
    }/*сброс ошибок*/
  }
}

/********* Усреднения часовые и суточные для дополн точки **********/
void AverageExpandParams (void)
{
  unsigned char i,j;
  for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2] == 3) for (j=0;j<2;j++)
    Expand.dyn[exp_dyn[i][0]+j+1]=(Expand.dyn[main_dyn[i][0]+j+1]*
      Expand.cnt[j]+Expand.dyn[exp_dyn[i][0]])/(Expand.cnt[j]+1);
  for (j=0;j<2;j++) Expand.cnt[j]++;
}

/***********  чтение из архива часового или суточного **************/
void ReadFromArchive (unsigned char bufer[])
{
  unsigned char i,j,val,num_page,buf_arc[146],typ_arc;
  int pnt_arc;
  unsigned segf,adrf;
  num_page=bufer[7];
  pnt_arc=bufer[8]*256+bufer[9];
  typ_arc=bufer[10];
  bufer[11]=0;
  switch (num_page)
  {
    case 0:segf=0xd000;break;
    case 1:segf=0xe000;break;
    case 2:segf=0xc000;break;
  }
  for (i=0;;)
  {
    adrf=pnt_arc*Size_str;
    if (FlashRead(segf,adrf)==typ_arc || typ_arc==255)
    {
      for (j=0;j < Size_str;j++)
      { 
        val=FlashRead(segf,adrf+j);
        bufer[12+i*Size_str+j]=val;
      } 
      i++;
    }  
    pnt_arc--;
    if (pnt_arc < 0)
    { /*обнаружен конец страницы, переключение на следующую*/
      switch (num_page)
      {
	      case 0:
          num_page=2;
          segf=0xc000;
	        pnt_arc=ReadNVRAM(17)*256+ReadNVRAM(18);
        break;
	      case 1:
          num_page=0;
          segf=0xd000;
	        pnt_arc=ReadNVRAM(13)*256+ReadNVRAM(14);
        break;
	      case 2:
          num_page=1;
          segf=0xe000;
	        pnt_arc=ReadNVRAM(15)*256+ReadNVRAM(16);
        break;
      } 
      if (pnt_arc == 0) 
      {
        bufer[11]=1;
        break;
      }
      if (num_page == ReadNVRAM(12)) 
      {
        bufer[11]=1;
        break;
      }/*не текущая*/
    } 
    if (((i+1)*Size_str+16) > 256) break;
  } 
  bufer[7]=i;bufer[8]=num_page;
  bufer[9]=pnt_arc/256;
  bufer[10]=pnt_arc-bufer[9]*256;
  bufer[6]=16+i*Size_str;
}

/********************  чтение из архива минутного *****************/
void ReadFromMinArch (unsigned char bufer[])
{
  unsigned char i,j,buf_arc[146];
  unsigned adrf;
  int pnt_arc;
  pnt_arc=bufer[7];
  bufer[11]=0;
  for (i=0;;)
  {
    adrf=pnt_arc*Size_str;
    X607_ReadFn(0x6000+adrf,Size_str,buf_arc);
    for (j=0;j < Size_str;j++) bufer[12+j+Size_str*i]=buf_arc[j];i++;
    if (pnt_arc == ReadNVRAM(21)) 
    { 
      bufer[11]=1;
      break;
    }
    pnt_arc--;
    if (pnt_arc < 0) pnt_arc=59;
    if (((i+1)*Size_str+16) > 256) break;
  } 
  bufer[7]=i;
  bufer[8]=pnt_arc;
  bufer[6]=16+i*Size_str;
}

/************* чтение событий,алармов,внешт.ситуаций *************/
void ReadFromEvents (unsigned char buf_com[])
{
  int adr_evt;
  unsigned char buf_evt[16],i,j,type;
  adr_evt=buf_com[8]*256+buf_com[9];
  type=buf_com[10];
  buf_com[10]=0;
  if (adr_evt < 512) for (i=0;;)
  {
    if (adr_evt==(ReadNVRAM(6+type*2)*256+ReadNVRAM(7+type*2)))
    {
      buf_com[10]=1;
      break;
    }
    X607_ReadFn(type*0x2000+adr_evt*16,16,buf_evt);
    if ((buf_evt[1] > 0)&&(buf_evt[2] > 0)&&
      (buf_evt[1] < 13)&&(buf_evt[2] < 32))
    { 
      for (j=0;j<16;j++) buf_com[11+i*16+j]=buf_evt[j];
      i++;
    }
    adr_evt--;
    if (adr_evt < 0) adr_evt=511;
    if (i >= 15) break;
  } 
  buf_com[6]=255;
  buf_com[7]=i;
  buf_com[8]=adr_evt/256;
  buf_com[9]=adr_evt-buf_com[8]*256;
}

/*********** визуализация ошибок *********************************/
void ViewError ()
{
  unsigned char i,buf[16];
  if (ind_err>=Max_error) 
  {
    ind_err=0;
  }/*визуализация ошибки*/
M:if (Err[ind_err] < 10)
  {
    if (flg_err[ind_err] != 0)
    {
	    flg_err[ind_err]=0;
      FormateEvent(buf);
      buf[13]=ind_err+1;
	    WriteEvent(buf,2);/*запись о сбросе нештатной ситуации*/
    } 
    ind_err++;
    if (ind_err < Max_error) goto M;
  } 
  else
  { /*показывает слово "Err" и код ошибки*/
    if (flg_err[ind_err] == 0)
    {
	    flg_err[ind_err]=1;
      FormateEvent(buf);
      buf[10]=1;
      buf[13]=ind_err+1;
	    WriteEvent(buf,2);/*запись об установке нештатной ситуации*/
    } 
    i=(ind_err+1)/10;
    i=ind_err+1-i*10;
    ind_err++;
  }
}

/************ возвращает конфигур.страницу Modbus ****************/
void ReadConfigModbus (unsigned char buf_com[])
{
  unsigned char i,buf_modbus[240],status;
  if (buf_com[7] != 15)
  {
    X607_ReadFn(0xa000+buf_com[7]*256,240,buf_modbus);
    for (i=0;i<240;i++) buf_com[8+i]=buf_modbus[i];
    buf_com[6]=252;
  } 
  else
  {
    for (i=0;i<30;i++)
    { 
      X607_ReadFn(0xaa00+i*8+7,1,&status);
      buf_com[8+i]=status;
    }
    buf_com[6]=42;
  }
}

/************ изменяет страницу конфиг.Modbus,запись события *******/
void WriteConfigModbus(unsigned char buf_com[])
{
  unsigned char i,buf_modbus[240],buf_evt[16];
  for (i=0;i<240;i++) buf_modbus[i]=buf_com[8+i];
  X607_WriteFn(0xa000+buf_com[7]*256,240,buf_modbus);
  buf_com[6]=11;
  FormateEvent(buf_evt);
  buf_evt[15]=13;
  buf_evt[14]=buf_com[7];
  WriteEvent(buf_evt,0);
}

/***************** переустановка порта ***************************/
void ReinstallPort (unsigned char number)
{
  unsigned char baud_,data_,stop_,parity_,details;
  details=Device.com[number-1][0];
  Port[number-1].reinst=0;
  Port[number-1].ta=set_ta[Device.com[number-1][1]];
  baud_=details & 15;
  data_=(details & 16) >> 4;
  stop_=(details & 32) >> 5;
  parity_=(details & 192) >> 6;
  InstallCom(number,set_bd[baud_],set_dt[data_],set_pr[parity_],
	set_st[stop_]);
}

/********** получение нижней и верхней границ по номеру шкалы *******/
void GetValuesScale (unsigned char num_scale,float *lo,float *hi)
{
  unsigned char buf_val[8];
  float border;
  X607_ReadFn(0x0ad00+(num_scale-1)*8,8,buf_val);
  ConvToFloatVerify(&border,buf_val[0],buf_val[1],buf_val[2],buf_val[3]);
  *lo=border;
  ConvToFloatVerify(&border,buf_val[4],buf_val[5],buf_val[6],buf_val[7]);
  *hi=border;
}

/************** возвращает программные описатели *****************/
void GetAllDescript (unsigned char buf[])
{
  unsigned char i,j,opt,flag,cr[4];
  flag=0;opt=buf[7];
  switch (opt)
  {
    case 0:
      for (i=0;i < Max_conf;i++) for (j=0;j<3;j++)
	      buf[11+i*3+j]=conf_basic[i][j];
      buf[7]=3;
	    buf[9]=Max_conf;
      buf[6]=15+Max_conf*3;
      flag=1;
    break;
    case 1:
      for (i=0;i < Max_conf;i++) for (j=0;j<3;j++)
	      buf[11+j+i*3]=conf_basic[i][j+3];
      buf[9]=Max_conf;
	    buf[7]=3;buf[6]=15+Max_conf*3;
      flag=1;
    break;
    case 2:
      for (i=0;i<Max_select;i++)
	      buf[9+i]=conf_select[i];
	    buf[6]=13+Max_select;
      flag=1;
    break;
    case 3:
      for (i=0;i < Max_dyn_massive;i++) for (j=0;j<4;j++)
	    {
	      if (j==3 && dyn_set[i][j]>=128 && Device.units<2)
	        buf[11+j+i*4]=sel_units[dyn_set[i][j]-128][Device.units];
	      else buf[11+j+i*4]=dyn_set[i][j];
	    } 
      buf[9]=Max_dyn_massive;
	    buf[7]=4;
      buf[6]=15+Max_dyn_massive*4;
      flag=1;
    break;
    case 4:
      for (i=0;i < Max_dyn_massive;i++) for (j=0;j<4;j++)
	      buf[11+j+i*4]=dyn_set[i][j+5];
      buf[9]=Max_dyn_massive;
      buf[7]=4;
	    buf[6]=15+Max_dyn_massive*4;
      flag=1;
    break;
    case 5:
      for (i=0;i < Max_main;i++) for (j=0;j<3;j++)
	      buf[11+j+i*3]=conf_main[i][j];buf[7]=3;
	    buf[9]=Max_main;
      buf[6]=15+Max_main*3;
      flag=1;
    break;
    case 6:
      for (i=0;i < Max_main;i++) for (j=0;j<3;j++)
	      buf[11+j+i*3]=conf_main[i][j+3];buf[9]=Max_main;
	    buf[7]=3;
      buf[6]=15+Max_main*3;
      flag=1;
    break;
    case 7:
      for (i=0;i<Max_select_main;i++)
	      buf[9+i]=main_select[i];
	    buf[6]=13+Max_select_main;
      flag=1;
    break;
    case 8:
      for (i=0;i < Max_archive;i++) for (j=0;j<4;j++)
	    {
	      if (j==0 && set_archive[i][j]>=128 && Device.units<2)
	        buf[11+j+i*4]=sel_units[set_archive[i][j]-128][Device.units];
	      else buf[11+j+i*4]=set_archive[i][j];
	    } 
      buf[9]=Max_archive;
      buf[7]=4;
	    buf[6]=15+Max_archive*4;
      flag=1;
    break;
    case 9:
      for (i=0;i < Max_icp_prm;i++) for (j=0;j<3;j++)
	      buf[11+j+i*3]=conf_icp[i][j+3];
      buf[9]=Max_icp_prm;
	    buf[7]=3;buf[6]=15+Max_icp_prm*3;
      flag=1;
    break;
    case 10:
      for (i=0;i < Max_icp_prm;i++) for (j=0;j<3;j++)
	      buf[11+i*3+j]=conf_icp[i][j];buf[7]=3;
	    buf[9]=Max_icp_prm;
      buf[6]=15+Max_icp_prm*3;
      flag=1;
    break;
    case 11:
      for (i=0;i < Max_exp_prm;i++) for (j=0;j<3;j++)
	      buf[11+i*3+j]=conf_exp[i][j];buf[7]=3;
	    buf[9]=Max_exp_prm;
      buf[6]=15+Max_exp_prm*3;
      flag=1;
    break;
    case 12:
      for (i=0;i < Max_exp_prm;i++) for (j=0;j<2;j++)
	      buf[11+i*2+j]=conf_exp[i][j+3];buf[7]=2;
	    buf[9]=Max_exp_prm;
      buf[6]=15+Max_exp_prm*2;
      flag=1;
    break;
    case 13:
      for (i=0;i < Max_exp_dyn;i++) for (j=0;j<5;j++)
	      buf[11+i*5+j]=exp_dyn[i][j];buf[7]=5;
	    buf[9]=Max_exp_dyn;
      buf[6]=15+Max_exp_dyn*5;
      flag=1;
    break;
    case 14:
      for (i=0;i < 8;i++) buf[11+i]=serial_num[i];
	    ConvLongToBynare(checksum,cr);
      buf[9]=1;
	    for (i=0;i < 4;i++) buf[19+i]=cr[i];
      buf[23]=Typ_task;
	    buf[24]=Max_pnt;
      buf[25]=Max_mvs;
      buf[26]=Max_icp;
	    for (i=0;i < 8;i++) for (j=0;j < 6;j++) /*было 31*/
	      buf[27+i*6+j]=conf_menu[i][j];buf[75]=Device.contrh;
	    for (i=0;i < 4;i++) buf[76+i]=typ_port[i];
	    for (i=0;i < 8;i++) buf[80+i]=ind_prm_alm[i];
	    buf[6]=92;
      flag=1;
    break;
  } 
  if (flag==0) 
  {
    buf[9]=0;
    buf[6]=15;
  }
}

/*********** изменение конфигурационного параметра ***************/
unsigned char WriteConfigParam (unsigned char size,unsigned char addr,
	  unsigned char type,unsigned char buf_com[],
	  unsigned char ind,unsigned char max_param,unsigned char num)
{
  unsigned char buf_evt[16],j,flag;
  FormateEvent(buf_evt);buf_evt[15]=buf_com[8]+1;
  buf_evt[14]=buf_com[9+ind];flag=0;
  if (buf_com[9+ind]<max_param && size>0) for (j=0;j<size;j++)
  {
    if (type==0 || type==10)
    {
      if (j<8) buf_evt[6+j]=buf_com[10+ind+j];
    } 
    else
    { /*если тип данных не строка*/
      buf_evt[6+j]=ReadEEP(buf_com[8],addr+j);
      buf_evt[10+j]=buf_com[10+ind+j];
    }
    if (buf_com[10+ind+j] != ReadEEP(buf_com[8],addr+j))
    {
      EnableEEP();WriteEEP(buf_com[8],addr+j,buf_com[10+ind+j]);
      ProtectEEP();flag=1;
    }
  } 
  if (flag==1)
  {
    WriteEvent(buf_evt,0);
    if (type==6) Port[num].reinst=1;
    if (type==8) flg_init_arc=1;
    if (type == 8) flag=0; 
    else flag=1;
  } 
  return flag;
}

/**************************/
void GetModuleAin (unsigned char buf_com[],struct modul_ai Ain)
{
  unsigned char i,j,k,cr[4];
  if (buf_com[8] == 0)
  {   /*получение установок*/
    for (i=0;i<6;i++) buf_com[10+i]=Ain.status[i];
    for (i=0;i<8;i++)
    {
      ConvToBynare(Ain.lo_brd[i],cr);
      for (k=0;k< 4;k++) buf_com[16+i*4+k]=cr[k];
      ConvToBynare(Ain.hi_brd[i],cr);
      for (k=0;k< 4;k++) buf_com[48+i*4+k]=cr[k];
      buf_com[80+i]=Ain.units[i];
      for (j=0;j<5;j++)
      {
	      ConvToBynare(Ain.alm_set[i][j],cr);
	      for (k=0;k< 4;k++) buf_com[88+j*4+i*20+k]=cr[k];
      } 
      buf_com[248]=Ain.alm_enb;
    } 
    buf_com[6]=253;
  } 
  else
  {  /*получение данных и статусов алармов*/
    for (i=0;i<8;i++)
    {
      ConvToBynare(Ain.prm[i],cr);
      for (k=0;k< 4;k++) buf_com[10+i*4+k]=cr[k];
      buf_com[42+i]=Ain.alm_status[i];
    } buf_com[6]=54;
  }
}

/************ получение данных от модуля аналоговых выходов  ************/
void GetModuleAout (unsigned char buf_com[],struct modul_ao aout)
{
  unsigned char i,k,cr[4];
  for (i=0;i<6;i++) buf_com[10+i]=aout.status[i];
  for (i=0;i<4;i++)
  {
    ConvToBynare(aout.lo_brd[i],cr);
    for (k=0;k< 4;k++) buf_com[16+i*4+k]=cr[k];
    ConvToBynare(aout.hi_brd[i],cr);
    for (k=0;k< 4;k++) buf_com[32+i*4+k]=cr[k];
    buf_com[48+i]=aout.units[i];
  }
  for (i=0;i<4;i++)
  {
    ConvToBynare(aout.prm[i],cr);
    for (k=0;k< 4;k++) buf_com[52+i*4+k]=cr[k];
  } 
  buf_com[6]=72;
}

/************ получение данных от модуля дискретных вх/вых ******/
void GetModuleDio (unsigned char buf_com[],struct modul_dio discr)
{
  unsigned char i;
  for (i=0;i<4;i++) buf_com[10+i]=discr.status[i];
  buf_com[14]=discr.inp;
  buf_com[15]=discr.stat_out;
  buf_com[6]=20;
}

/*********** получение данных от модуля счётчиков  ************/
void GetModuleCounters (unsigned char buf_com[])
{
  unsigned char i,j, cr[4];
  for (i=0;i<4;i++) buf_com[10+i]=Counters.status[i];
  for (i=0;i<2;i++)
  {
    ConvToBynare(Counters.counter[i],cr);
    for (j=0;j< 4;j++) buf_com[14+i*4+j]=cr[j];
    ConvToBynare(Counters.value[i],cr);
    for (j=0;j< 4;j++) buf_com[22+i*4+j]=cr[j];
    ConvToBynare(Counters.factor[i],cr);
    for (j=0;j< 4;j++) buf_com[30+i*4+j]=cr[j];
  } 
  buf_com[38]=Counters.start[0];
  buf_com[39]=Counters.start[1];
  buf_com[6]=44;
}

/************* выполнение инициализации структур **************/
void ExecuteInitialize (unsigned char *flag)
{
  unsigned char j;
  switch (*flag)
  {
    case 1: case 2: case 3: case 4:
      InitBasicStruct(*flag-1,&Config[*flag-1],&Basic[*flag-1]);break;
    case 5:Real_exp_dyn=SetExpandDescript();break;
    case 7:
      for (j=0;j<Max_icp_ain;j++) InitModuleStruct(j);
      for (j=0;j<Max_icp_aout;j++) InitModuleOutStruct(j);
    break;
    case 8:InitStationStruct(&Device);Script.count=0;break;
  } 
  *flag=0;
}

/************* запись кода скрипта в ЭНП, запись события *******/
void WriteCodeScript (unsigned char buf_com[])
{
  unsigned char i,j,buf_script[6],buf_evt[16];
  unsigned addr;
  addr=buf_com[7]*256+buf_com[8];
  if (addr<640 && buf_com[9]<=40)
    for (i=0;i<buf_com[9];i++)
    {
      for (j=0;j<6;j++) buf_script[j]=buf_com[10+i*6+j];
      X607_WriteFn(0xb000+(addr+i)*6,6,buf_script);
    } 
  buf_com[6]=11;
  FormateEvent(buf_evt);
  buf_evt[15]=13;
  buf_evt[14]=14;
  WriteEvent(buf_evt,0);
}

/************* чтение кода скрипта ******************************/
void ReadCodeScript (unsigned char buf_com[])
{
  unsigned char i,j,buf_script[6];
  unsigned addr;
  addr=buf_com[7]*256+buf_com[8];
  if (addr < 640 )
  {
    for (i=0;i<40;i++)
    {
      X607_ReadFn(0xb000+(addr+i)*6,6,buf_script);
      for (j=0;j<6;j++) buf_com[10+i*6+j]=buf_script[j];
    } 
    buf_com[9]=40;
    buf_com[6]=254;
  } 
  else 
  {
    buf_com[9]=0;
    buf_com[6]=14;
  }
}

/*********** обработка коммуник.запроса и выдача ответа **********/
void CommunnicLink (unsigned char buf[],unsigned char num_port)
{
  int adr_evt;
  unsigned segment;
  float value;
  unsigned char i,k,j,n,c,m,count,buf_evt[16],cr[4];
  unsigned char flg_passw,flag;
  int val;
  unsigned long ks;
  if (buf[0] != 240) if (buf[0] != Device.addr) goto M;
  i=buf[6]-4; 
  CalcCRC32(buf,buf[6]-4,&ks); 
  flg_init=0;
  ConvLongToBynare(ks,cr); 
  k=0;
  for (j=0;j<4;j++) if (buf[i+j] != cr[j]) 
  {
    k=1;
    break;
  }
  if (k == 0) /*нет ошибки циклического кода*/
  {
    if ((buf[5] & 1)==0)  flg_passw=1;
    else
    {
      k=0;
      for (j=1;j<4;j++)	if (buf[j+1]!=Device.passw[j]) 
      { 
        k=1;
        break;
      }
      if (k==1) flg_passw=0;
      else flg_passw=1;
    } 
    FormateEvent(buf_evt);
    if (flg_passw==1) 
      switch (buf[5])
      { 
        case 1: /* команда изменения конфигурации */
	        if (buf[7]>0)
	        {
	          i=0;
	          if (buf[8]==7) for (k=0;k<buf[7];k++)
	          {
	            flag=WriteConfigParam(conf_main[buf[9+i]][1],
		            conf_main[buf[9+i]][2],conf_main[buf[9+i]][3],buf,i,
		              Max_main,conf_main[buf[9+i]][4]);
	            if (flag == 1) flg_init=8;
	            i=i+conf_main[buf[9+i]][1]+1;
	          }
            else if (buf[8]==6) for (k=0;k<buf[7];k++)
	          {
	            flag=WriteConfigParam(conf_icp[buf[9+i]][1],
		            conf_icp[buf[9+i]][2],conf_icp[buf[9+i]][3],buf,i,
		              Max_icp_prm,conf_icp[buf[9+i]][4]);
	            if (flag == 1) flg_init=7;
	            i=i+conf_icp[buf[9+i]][1]+1;
	          } 
            else if (buf[8]==4) for (k=0;k<buf[7];k++)
	          {
	            flag=WriteConfigParam(conf_exp[buf[9+i]][1],
		            conf_exp[buf[9+i]][2],conf_exp[buf[9+i]][3],buf,i,
		              Max_exp_prm,conf_exp[buf[9+i]][4]);
	            if (flag == 1) flg_init=5;
	            i=i+conf_exp[buf[9+i]][1]+1;
	          } 
            else if (buf[8]<4) for (k=0;k<buf[7];k++)
	          {
	            flag=WriteConfigParam(conf_basic[buf[9+i]][1],
		            conf_basic[buf[9+i]][2],conf_basic[buf[9+i]][3],buf,i,
		              Max_conf,conf_basic[buf[9+i]][4]);
	            if (flag == 1) flg_init=buf[8]+1;
	            i=i+conf_basic[buf[9+i]][1]+1;
	          }
	        } 
          buf[6]=12;
          buf[7]=num_port;
        break;
        case 2:/* команда чтения конфигурации */
	        j=buf[7];
          if (j < 8) for (i=0;i< size_conf[j];i++)
	          buf[i+7]=ReadEEP(j,i);
          buf[6]=11+size_conf[j];
        break;
        case 3: /* команда изменения даты и времени запись события */
	        buf_evt[14]=1;
          buf_evt[15]=9;
	        for (j=0;j<6;j++) buf_evt[6+j]=buf[7+j];
          WriteEvent(buf_evt,0);
	        SetTime(buf[10],buf[11],buf[12]);
	        SetDate(buf[7]+2000,buf[8],buf[9]);
          buf[6]=11;
        break;
        case 4:
          for (i=0;i< 6;i++) buf[i+7]=ReadNVRAM(i);
          buf[6]=17;
	      break; /* команда чтения даты и времени */
        case 5:
          if ((buf[7] < Max_pnt)&&(buf[8] > 0)) /*выполнить поверку точности*/
	        {
	          for (i=0;i< buf[8];i++)
	          {
		          if (Max_dyn_massive > buf[9+i*5])
		            ConvToFloatVerify(&Basic[buf[7]].dyn[dyn_set[buf[9+i*5]][0]+
		              dyn_set[buf[9+i*5]][1]],buf[10+i*5],buf[11+i*5],
				            buf[12+i*5],buf[13+i*5]);
	          } 
            Config[buf[7]].flag=1;
	        } 
          buf[6]=11;
        break;
        case 6:
          if (buf[7]<Max_pnt)
	        { /*чтение расчётных параметров*/
	          n=0;
            for (i=0;i< Max_dyn_massive;i++) if (dyn_set[i][5]==1)
	          {
		          value=Basic[buf[7]].dyn[dyn_set[i][0]+dyn_set[i][1]];
		          ConvToBynare(value,cr);
		          for (j=0;j< 4;j++) buf[8+n*4+j]=cr[j];
              n++;
	          } 
            buf[7]=n;
            buf[6]=12+n*4;
	        } 
          else 
          {
            buf[7]=0;
            buf[6]=12;
          } 
        break;

        //15.09.2021 
        /*case 7:  /* изменение адреса,тэга,единиц MVS */
	        /*if (buf[7] < Max_mvs)
	        {
	          j=buf[7];
            Sensor[j].setup[0]=buf[8];
	          for (i=0;i<10;i++) Sensor[j].setup[3+i]=buf[10+i];
	          Sensor[j].evt=2;
            Sensor[j].setup[1]=buf[9];
	        } 
          buf[6]=11;
        break;*/
        case 8:
          for (i=0;i<16;i++) buf[7+i]=ReadNVRAM(6+i);
	        buf[6]=27;
        break;
        //15.09.2021
        /* чтение указателей архивов*/
        /*case 9:
          if (buf[7] < Max_mvs)
	        {
	          j=buf[7];
            mvs_cmd[0]=buf[8];
            mvs_cmd[1]=buf[9];
	          for (i=0;i<4;i++) mvs_val[i]=buf[10+i];
            Sensor[j].evt=3;
	        } 
          buf[6]=11;
        break;*/
        case 10:  ReadFromEvents(buf);break;
        case 11:
          if (buf[7]< Max_icp)
	        { /*записать в модуль новую конфиг или калибровки*/
		        switch (Device.typ_icp[buf[7]])
		        {
		          case 0:Ain[0].evt=buf[8];break;
		          case 1:Ain[1].evt=buf[8];break;
		          case 2:Aout[0].evt=buf[8];break;
		          case 3:Aout[1].evt=buf[8];break;
		          case 6:Counters.evt=buf[8];break;
		        } 
            for (i=0;i<3;i++) icp_wr[i]=buf[9+i];
	        } 
          buf[6]=11;
        break;
        case 12:ReadFromArchive(buf);break;/*суточный архив*/
        case 15:
          if (buf[7]<4 && buf[8]>0) for (i=0;i<buf[8];i++)
	          if (buf[9+i]<Max_dyn_massive) /*очистка счётчиков*/
	          {
		          value=Basic[buf[7]].dyn[dyn_set[buf[9+i]][0]+dyn_set[buf[9+i]][1]];
		          ConvToBynare(value,cr);
		          buf_evt[15]=10;
              buf_evt[14]=buf[9+i];
              buf_evt[13]=buf[7];
		          for (j=0;j<4;j++) buf_evt[6+j]=cr[j];
              WriteEvent(buf_evt,0);
		          Basic[buf[7]].dyn[dyn_set[buf[9+i]][0]+dyn_set[buf[9+i]][1]]=0.0;
	          } 
          buf[6]=11;
        break;

        //15.09.2021 пока без изменений
        case 16:
          if (buf[7] < Max_mvs)
	        { /* длинное чтение MVS */
		        j=buf[7];
            for (i=0;i<13;i++) buf[7+i]=Sensor[j].setup[i];
		        for (i=0;i<21;i++)
		        {
		          ConvToBynare(Sensor[j].data[i],cr);
		          for (k=0;k< 4;k++) buf[20+i*4+k]=cr[k];
		        } 
            for (i=0;i<3;i++) for (n=0;n<3;n++)
		        {
		          ConvToBynare(Sensor[j].alm_set[i][n],cr);
		          for (k=0;k< 4;k++) buf[104+n*4+i*12+k]=cr[k];
		        } 
            buf[140]=Sensor[j].alm_enb;buf[6]=145;
	        } 
          else 
            buf[6]=11;
        break;
        case 17:WriteCodeScript(buf);break;

        //15.09.2021 пока без изменений
        case 18:
          if (buf[7] < Max_mvs)
	        { /* короткое чтение MVS */
		        j=buf[7];
            buf[7]=Sensor[j].setup[2];
		        for (i=0;i<4;i++)
		        {
		          ConvToBynare(Sensor[j].data[i],cr);
		          for (k=0;k< 4;k++) buf[8+i*4+k]=cr[k];
		        }
	        } 
          buf[6]=28;
        break;

        case 19:if (buf[7] == 1) InitModem();buf[6]=11;break;
        case 20:
          if (buf[7] < 4) /*просмотр параметров основных точек*/
	        {
		        for (j=0;j<Max_dyn_massive;j++)
		        {
		          value=Basic[buf[7]].dyn[dyn_set[j][0]+dyn_set[j][1]];
		          ConvToBynare(value,cr);
		          for (k=0;k< 4;k++) buf[9+k+j*4]=cr[k];
		        } 
            buf[6]=13+Max_dyn_massive*4;
            buf[8]=Max_dyn_massive;
	        } 
          else if (buf[7]==4)
	        {
		        n=flag=0;
		        for (j=0;j<Real_exp_dyn;j++)
		        {
		          if (exp_dyn[j][2]==1 || exp_dyn[j][2]==3)
		            c=1;
              else if (exp_dyn[j][2]==2) c=7;
		          else if (exp_dyn[j][2]==4) c=5;
              else c=0;
		          if (c > 0) for (m=0;m<c;m++)
		          {
		            ConvToBynare(Expand.dyn[exp_dyn[j][0]+m],cr);
		            for (k=0;k< 4;k++) buf[9+k+n*4]=cr[k]; 
                n++;
		            if (n >= 60) {flag=1;break;}
		          } 
              if (flag == 1) break;
		        } 
            buf[6]=13+n*4; 
            buf[8]=n;
	        } 
          else 
          {
            buf[6]=13; 
            buf[8]=0;
          } 
        break;
        case 21:WriteConfigModbus(buf);break;
        case 22:
          if (buf[7] < Max_icp)
	        {
		        switch (Device.typ_icp[buf[7]])
		        {  /*тип,формат,имя модуля*/
		          case 0:GetModuleAin(buf,Ain[0]);break;
		          case 1:GetModuleAin(buf,Ain[1]);break;
		          case 2:GetModuleAout(buf,Aout[0]);break;
		          case 3:GetModuleAout(buf,Aout[1]);break;
		          case 4:GetModuleDio(buf,Dio[0]);break;
		          case 5:GetModuleDio(buf,Dio[1]);break;
		          case 6:GetModuleCounters(buf);break;
		          default:buf[6]=12;break;
		        }
		        buf[8]=Device.adr_icp[buf[7]];
            buf[9]=Device.typ_icp[buf[7]];
	        } 
          else buf[6]=12;
        break;
        case 24:ReadCodeScript(buf);break;
        case 28:ReadFromMinArch(buf);break;/*читать минутный архив*/
        case 30:ReadConfigModbus(buf);break;
        case 32:GetAllDescript(buf);break;
        case 33:WriteAlarmsSetup(buf);break;

        //15.09.2021
        case 36:GetDataEcoSonic(buf);break;
        default:buf[5]=254;buf[6]=11;break;
      } 
    else 
    { 
      buf[5]=255;
      buf[6]=11;
    };
    CalcCRC32(buf,buf[6]-4,&ks);
    ConvLongToBynare(ks,cr);
    count=buf[6]-4;
    buf[count]=cr[0];
    buf[count+1]=cr[1];
    buf[count+2]=cr[2];
    buf[count+3]=cr[3];
    if (num_port == 3)
    {
      ToComBufn_3(buf,buf[6]);
      if (Port[2].reinst==1)
        WaitTransmitOver(num_port);
    }
    if (num_port == 4)
    {
      ToComBufn_4(buf,buf[6]);
      if (Port[3].reinst==1 && Device.set_com==1)
        WaitTransmitOver(num_port);
      else Port[3].reinst=0;
    }
  } M:
}

/*************** настройки конфигурации в терминал *********************/
void ConfigSetToMMI (unsigned char config[],unsigned char select[])
{
  unsigned char i,j,k;
  for (i=0;i<7;i++) coord[i]=config[i];k=0;
  if (coord[3] == 8)
  { /*поиск индекса имени архивной переменной*/
    j=ReadEEP(Display.point,coord[2]+2);
    for (i=0;i<Max_dyn_massive;i++)
    if (dyn_set[i][0] == j) 
    { 
      coord[7]=dyn_set[i][2];
      break; 
    }
  }
  if (coord[3]==2 && select[0]>0)
  {
    for (i=0;i<select[0];i++)
    {
      if (select[1+k]==Display.prm && select[2+k]>0)
      {
	      for (j=0;j<select[2+k];j++) mmi_sel[j]=select[3+k+j];
        break;
      } 
      k=k+select[2+k]+2;
    } 
    mmi_num_sel=select[2+k];
  }
}

/*********** формирование параметра просмотра для терминала ************/
void ViewParamToMMI (double *value)
{
  unsigned char i,j,k,flag;
  if (Display.page==14)
  {
    if (Display.point < 4)
    {
      size_max=Max_dyn_massive;
      if ((Display.num+Display.row)<size_max)
      {
	      if (mmi_pass==1)
	        *value=Basic[Display.point].dyn[dyn_set[Display.num+Display.row][0]+
				    dyn_set[Display.num+Display.row][1]];
        else
	      {
	        coord[0]=dyn_set[Display.num+Display.row][2]; 
          coord[4]=0;
	        coord[1]=dyn_set[Display.num+Display.row][1];
	        coord[2]=dyn_set[Display.num+Display.row][4];
	        coord[3]=dyn_set[Display.num+Display.row][3];
	      }
      }
    }
    else if (Display.point==5 || Display.point==6)
    {
      size_max=8; 
      if ((Display.num+Display.row)<size_max)
        if (mmi_pass==1)
	        *value=Ain[Display.point-5].prm[Display.num+Display.row];
        else
	      {
	        coord[0]=50;
          coord[1]=0;
          coord[2]=0;
          coord[3]=0;
          coord[4]=1;
	        coord[5]=Display.num+Display.row+1;
	      }
      } 
      else if (Display.point>=7 && Display.point<=10)
      {
        size_max=3; 
        if ((Display.num+Display.row)<size_max)
          if (mmi_pass==1)
	          *value=Sensor[Display.point-7].avg[Display.num+Display.row];
          else
          {
	          coord[0]=Display.num+Display.row+1;
	          coord[1]=coord[2]=coord[3]=coord[4]=0;
	        }
      } 
      else if (Display.point==11)
      {
        size_max=2; 
        if (Display.row<size_max) if (mmi_pass==1)
          *value=Counters.counter[Display.row];
        else
        {
	        coord[0]=50;
          coord[1]=0;
          coord[2]=0;
          coord[3]=0;
          coord[4]=1;
	        coord[5]=Display.row+1;
        }
      }
    } 
    else if (Display.page==16 && Display.flag==1)
    {
      if (Display.point<4)
      {
        size_max=Max_conf;
        ConfigSetToMMI(conf_basic[Display.prm],conf_select);
      } 
      else if (Display.point==4)
      {
        size_max=Max_exp_const;
        coord[5]=0;
        coord[6]=1;
        for (i=0;i<5;i++) coord[i]=conf_exp[Display.prm+Real_exp_dyn][i];
      }  
      else if (Display.point==7)
      {
        size_max=Max_main;
        ConfigSetToMMI(conf_main[Display.prm],main_select);
      } 
      else if (Display.point==6)
      {
        size_max=Max_icp_prm;
        for (i=0;i<7;i++) coord[i]=conf_icp[Display.prm][i];
      } 
      else if (Display.point==60)
      {
        size_max=5; 
        coord[0]=90+Display.prm; 
        coord[2]=Display.prm; 
        coord[3]=60;
        coord[4]=128; 
        coord[5]=0; 
        coord[6]=1;
      }
    } 
    else if (Display.page<10 && Display.flag==0)
    { 
      ConvLongToBynare(checksum,coord);
    } 
    else if (Display.page==17)
    { 
      *value=Device.passw_op;
    }
    else if (Display.page==13 && Display.flag==1)
    {
      size_max=Max_arch_pnt;
      k=0;
      if ((Display.num+Display.row)<Max_arch_pnt)
        for (i=0;i<Max_main;i++) if (conf_main[i][3]==8)
        {
          if (conf_main[i][4]==(Display.num+Display.row)) /*номер для однотип*/
          {
	          coord[0]=conf_main[i][0];
            coord[1]=conf_main[i][4]+1;
	          coord[2]=Device.arch[k][0];/*номер списка*/
	          if (coord[2] > 0)
          	{
	            coord[3]=Device.arch[k][1]; 
              coord[5]=Device.arch[k][3];
	            coord[6]=k;
	            if (coord[2] == 1) for (j=0;j<Max_dyn_massive;j++)
	              if (dyn_set[j][0]==Device.arch[k][2])
	            {
                coord[4]=dyn_set[j][2];
                break;
              }
	            if (coord[2] == 2) for (j=0;j<Max_exp_dyn;j++)
	              if (exp_dyn[j][0]==Device.arch[k][2])
	            {
                coord[4]=exp_dyn[j][3];
                break;
              }
	            break;
	          }
          }
          if (ReadEEP(7,conf_main[i][2]) > 0) k++;
        }
    }
    else if (Display.page==20 && Display.flag==1)
    {
      if (Display.point==7) 
      {
        Display.size=Max_main;
        if ((Display.num+Display.row)<Max_main) 
        {
	        coord[0]=conf_main[Display.num+Display.row][0];
	        coord[1]=conf_main[Display.num+Display.row][4];
        }
      } 
      else if (Display.point<=3) 
      {
        Display.size=Max_conf;
        if ((Display.num+Display.row)<Max_conf) 
        {
	        coord[0]=conf_basic[Display.num+Display.row][0];
	        coord[1]=conf_basic[Display.num+Display.row][4];
        }
      } 
      else if (Display.point==6)
      {
        Display.size=Max_icp_prm;
        if ((Display.num+Display.row)<Max_icp_prm) 
        {
	        coord[0]=conf_icp[Display.num+Display.row][0];
	        coord[1]=conf_icp[Display.num+Display.row][4];
        }
      } 
      else if (Display.point==60) 
      {
        Display.size=5;
        if ((Display.num+Display.row)<Display.size) 
        {
	        coord[0]=90+Display.row; coord[1]=128;
        }
      }
    }
}

/********* cохранение параметров точки учёта в ЭНП ***************/
void SaveParameters (unsigned char num_pnt,struct dynparams bs)
{
  unsigned char i,j,k,ind,cr[4],buf[Max_save];
  float value;ind=0;
  for (i=0;i<Max_dyn;i++) if (main_dyn[i][2] > 0)
    for (j=0;j<main_dyn[i][1];j++)
  {
    value=bs.dyn[main_dyn[i][0]+j];
    ConvToBynare(value,cr);
    for (k=0;k< 4;k++) buf[ind*4+k]=cr[k];ind++;
  } 
  X607_WriteFn(0x9000+num_pnt*Max_save,Max_save,buf);
}

/********* cохранение параметров дополнит точки в ЭНП ************/
void SaveExpParams (void)
{
  unsigned char i,j,k,cr[4],ind;ind=0;
  for (i=0;i<Real_exp_dyn;i++) if (exp_dyn[i][2]>0 && exp_dyn[i][2]<4)
    for (j=0;j<exp_dyn[i][1];j++)
  {
    ConvToBynare(Expand.dyn[exp_dyn[i][0]+j],cr);
    X607_WriteFn(0x9000+(Max_save+ind)*4,4,cr);ind++;
  }
}

/********* проверка контрольной суммы для протокола DCON *********/
unsigned char VerifySum (unsigned char buf[],unsigned char count)
{
  unsigned char cr[2],flag;
  CalcCheckSum(buf,count,cr);
  if ((ascii_to_hex(buf[count])==cr[0])&&
    (ascii_to_hex(buf[count+1])==cr[1])) flag=1;
  else flag=0;
  return flag;
}

/************ преобразоваие в символьную строку ******************/
/*unsigned char StringToPanel (float val,unsigned char buf_str[],
		    unsigned char attrib)
{
  unsigned char i,j,dot,count,length;
  float a,b,c;
  unsigned long d;
  count=0;
  if (attrib == 0) length=6;
  else length=7;
  if (val < 0.0) 
  {
    val=fabs(val);
    buf_str[count]=Key_minus;
    count=1;
  }
  if (val < 1000000)
  {
    b=c=1000000;
    a=1;
    dot=0;
    i=0;
    while (val/a>=10) 
    {
      a=a*10;
      dot++;
      b=b/10;
    } 
    val=val*b;
    while (c>=1)
    {
      d=val/c;
      j=d;
      buf_str[count]=hex_to_ascii[j & Key_mask];
      count++;
      if (i == (dot+attrib)) break;
      if (dot==i) 
      {
        buf_str[count]=Key_dot;
        count++;
      }
      val=val-d*c;
      c=c/10;i++;
    }
  } 
  if (count < length) for (i=0;i<count;i++) /*сдвиг вправо*/
  /*{
    buf_str[length-i-1]=buf_str[count-i-1];
    buf_str[count-i-1]=Key_blank;
  } 
  return length;
}

/************* выдача запроса на табло **************/
/*void SendToPanel (float val,unsigned char attrib)
{
  unsigned char buf_panel[10],i,bufs[16],cr[2],count;
  for (i=0;i<10;i++) buf_panel[i]=Key_blank;
  count=StringToPanel(val,buf_panel,attrib);
  panel_pool=1;
  bufs[0]=0x7e;
  bufs[1]=Key_0;
  bufs[2]=Key_1;
  bufs[3]=Key_T;
  bufs[4]=hex_to_ascii[(Panel.ind+1) & Key_mask];
  for (i=0;i<count;i++) bufs[5+i]=buf_panel[i];
  count=count+5;
  CalcCheckSum(bufs,count,cr);
  bufs[count]=hex_to_ascii[cr[0]];
  bufs[count+1]=hex_to_ascii[cr[1]];
  bufs[count+2]=Key_termin;
  count=count+3;
  ToComBufn(3,bufs,count);
}
/***************** читает номер страницы индикатора ********/
/*void ReadPageMMI (unsigned char buf_com[])
{
  unsigned char number;
  number=ascii_to_hex(buf_com[3])*16+
			     ascii_to_hex(buf_com[4]);
  if (number != Display.page) SetDisplayPage(Typ_task);
}*///nm