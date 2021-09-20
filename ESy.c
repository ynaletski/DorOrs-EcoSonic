/***** расчёт расхода газа диафр/аннуб, метод GERG-91,NX-19 ******/
/*10-06-09 введено ограничение на число Рейнольдса>1000 для диафр*/
/*26-06-09 введён массив индексов имён алармов ind_prm_alm[8]    */
#include<math.h>


#define Max_dyn_all        43   /**/
#define Max_save          128   /**/
#define Max_conf           19   /*число конфиг.параметров задачи*/ //20.09.2021 was: 43
#define Max_select        160
#define Max_dyn            11   //20.09.2021 was: 18
#define Max_dyn_massive    17   //20.09.2021 was: 24
#define Max_main           69   /*число конфиг.параметров станции*/
#define Max_select_main    64
#define Max_archive         7   /*число архив.параметров задачи*/
#define Max_arch_pnt       40   /*число архив.точек станции*/
#define Max_arch_record   167  /*макс длина архив записи в байтах*/
#define Max_config_float   29  /*число действительных конфиг.параметров*/
#define Max_icp_prm        74  /*число параметров настроек модулей*/
#define Max_exp_prm        60
#define Max_exp_dyn        40
#define Max_exp_const      20

#define Max_error         50  /*число типов ошибок*/
#define Max_mvs           4   /*число многопараметрических датчиков*/
#define Max_icp           4   /*число модулей в/в*/
#define Typ_task          2   /*учёт газа новый гост на диафрагму*/
#define Max_icp_ain       2   /*число модулей аналог.ввода*/
#define Max_icp_aout      2   /*число модулей аналог.вывода*/
#define Max_icp_dio       2   /*число модулей дискрет ввода/вывода*/

struct dynparams
{
  float dyn[Max_dyn_all];
  unsigned int cnt[4];
};

struct expandparams
{
  float        dyn[256];
  unsigned int cnt[4];
};

struct modbus_config
{
  unsigned char mode;             /*0-Slave, 1-Host*/
  unsigned char protocol;         /*0-ASCII, 1-RTU*/
  unsigned char connect;          /*0-откл, 1-СОМ2, 2-СОМ3, 3-СОМ1*/
  unsigned char delay;
};

struct script
{
  double         accum;
  double	 argum;
  unsigned      count;
  unsigned char buf[6];
  unsigned       wait;
  unsigned char flag;
  unsigned     delay;
};

struct runtime
{
  unsigned char nmb;          /*порядковый номер модуля или датчика*/
  unsigned char nmb_icp;      /*порядковый номер модуля*/
  unsigned char ind;          /*для адресации модуля или датчика*/
  unsigned char pnl;          /*порядковый номер строки на табло*/
  unsigned char cnt_avg;      /*счётчик усреднений от датчиков*/
  unsigned char flg_err;      /*флажок визуализации ошибки*/
  unsigned char old_sec;
  unsigned char old_min;
  unsigned char old_hour;
  unsigned char old_month;
  unsigned char mmi;
};

struct modbus_host
{
  unsigned char num_pool;          /*порядковый номер запроса*/
  unsigned char stat_pool;         /*статус выполнения запроса*/
  unsigned char adr;               /*адрес внешнего устройства*/
  unsigned char func;              /*функция*/
  unsigned char count;             /*число регистров в запросе*/
  unsigned char status;            /*запрос включен,код ошибки*/
  unsigned      readr;             /*переадресатор*/
  unsigned char flag;            /*признак приостановки запросов*/
  unsigned      wait;            /*счётчик 50 мс интервалов*/
};

struct modul_dio
{
  unsigned char inp;
  unsigned char out;
  unsigned char stat_out;
  unsigned char status[4];
  unsigned char evt;
};

struct modul_ao
{
  float         prm[4];            /*параметры до 4-х каналов*/
  float         lo_brd[4];         /*нижняя граница диапазона*/
  float         hi_brd[4];         /*верхнняя граница диапазона*/
  unsigned char units[4];
  unsigned char status[6];
  unsigned char evt;
  unsigned char chnl;
};

struct modul_ai
{
  float         prm[8];            /*параметры до 8-ми каналов*/
  float         avg[8];            /*усредненные*/
  float         lo_brd[8];         /*нижняя граница диапазона*/
  float         hi_brd[8];         /*верхнняя граница диапазона*/
  float         alm_set[8][5];    /*уставки для каналов:нн,ниж,верх,вв,гистер,*/
  unsigned char units[8];
  unsigned char status[4];
  unsigned char evt;
  unsigned char alm_enb;           /*алармы разрешены/запрещены*/
  unsigned char alm_status[8];        /*факт регистрации аларма */
};

struct counters
{
  unsigned char status[4];
  unsigned char evt;
  unsigned char chan;
  unsigned char start[2];
  float        counter[2];
  float         factor[2];
  float         value[2];
};

struct mvs
{
  float data[21];              /*динамические параметры и зн.калибровки*/
  float avg[3];                /*усреднённые зн.дин.параметров*/
  unsigned char setup[13];     /*тег,адрес,статус,единицы*/
  unsigned char wait;           /*число временных интервалов паузы*/
  unsigned char evt;            /*событие*/
  unsigned char alm_enb;      /*алармы разрешены/запрещены*/
  float alm_set[3][3];         /* уставки для перепада:ниж,верх,гистер*/
  unsigned char alm_status[3];       /* факт регистрации аларма */
  //15.09.2021
  unsigned units[3];
  unsigned char err[3];
  unsigned char flag;
};

struct comport
{
  unsigned char buf[256]; /*буфер приёма*/
  unsigned char status; /*статус выполнения операции:
			 0-исходное,1-в буфере есть данные,2-принят блок*/
  unsigned char index;  /*индекс буфера приёма*/
  unsigned char timer;  /*счётчик временных интервалов*/
  unsigned char ta;     /*интервал тайм-аута кратный 50 мсек*/
  unsigned char reinst;
};

struct modbus_config       Modbus;
const unsigned char size_conf[8]={139,139,139,139,240,0,224,236};
const unsigned char etl[10] = {1,2,4,8,16,32,64,128,256,512};
const unsigned char ind_prm_alm[8]={26,27,29,30,32,33,24,25};
const unsigned char typ_port[4] = {3,2,1,1};
unsigned char ind_accum[4]={1,2,4,6};
/*гигадж или гигакал, гигадж в час или гигакал в час,
  килодж на кг или килокал на кг*/
const unsigned char sel_units[3][2]={{6,7},{15,16},{12,19}};
unsigned char exp_dyn[Max_exp_dyn][5];
float exp_const[Max_exp_const];
const unsigned char conf_menu[8][6]={
 {1,1,2,3,4,5},/*общие установки*/{1,6,7,8,0,0},/*первая точка*/
 {1,6,7,8,0,0},/*вторая точка*/   {1,6,7,8,0,0},/*третья точка*/
 {1,6,7,8,0,0},/*четвёртая точка*/{1,9,10,0,0,0},/*доп точка*/
 {1,11,0,0,0,0},/*модули в/в*/    {0,0,0,0,0,0}/*резерв*/};
/*[0]-начальный адрес,
/*[1]-размер смещения,
  [2]-признак сохранения в ЭНП:>1- да,2-накопление,3-усреднение*/
const unsigned char main_dyn[Max_dyn][3]={
 {0,4,2},/*Расход раб.*/  {4,4,3},/*Давление абс*/  {8,4,3},/*Температура*/ //20.09.2021 was: {0,4,3},/*Перепад*/      {4,4,3},/*Давление абс*/...
 {12,4,3},/*Плотность*/   {16,4,3},/*Азот*/         {20,4,3},/*Угл.газ*/
 {24,8,2},/*Расход ст.*/  {32,1,0},/*К сжимаемости*///{33,1,0},/*К потока*/           
 //{34,1,0},/*К расширения*/{35,1,0},/*К неостроты*/  {36,1,0},/*К шероховатости*/
 /*{37,1,0},/*К Рейнольдса*//*{38,1,0},*//*Число Рейнол*/ {39,1,0},/*Вязкость дин*/
 {40,1,0},/*Скорость*/    /*{41,1,0},/*П адиабаты*/   {42,1,0} /*Плот.раб*/
};
/*расшир.дин.параметры[0]-начальный адрес,[1]-индекс смещения,[2]-индекс имени
[3]-индекс единиц,[4]-признак накопленных,[5]-признак поверки:1-выходные,>1-исходные
[6]-модбас:64-в качестве ответчика,128-в качестве задатчика,192-оба+ном фун
  [7]-скрипты,[8]-признак очистки параметра*/
const unsigned char dyn_set[Max_dyn_massive][9]={{0,0,1,4,0,2,66,1,0}, //20.09.2021 was: ...][9]={{0,0,1,2,0,2,66,1,0},
 {4,0,2,2,0,3,194,1,0},{8,0,3,3,0,4,194,1,0},{12,0,4,8,0,0,194,1,0},
 {16,0,12,20,0,0,66,1,0},{20,0,13,20,0,0,66,1,0},{24,0,48,17,2,1,194,1,0},
 {24,1,48,18,2,0,66,0,1},{24,2,48,18,2,0,66,0,1},{24,3,48,18,2,0,66,0,1},
 {24,4,48,18,2,0,66,0,1},{24,5,48,18,2,0,66,0,1},{24,6,48,18,2,0,66,0,1},
 {32,0,20,0,0,1,0,0,0},/*{33,0,21,0,0,1,0,0,0},{34,0,22,0,0,1,0,0,0},
 {35,0,23,0,0,1,0,0,0},{36,0,24,0,0,1,0,0,0},{37,0,25,0,0,1,0,0,0},*/
 /*{38,0,26,0,0,1,0,0,0},*/{39,0,27,29,0,1,1,0,0},{40,0,28,10,0,1,1,0,0},
 /*{41,0,30,0,0,1,1,0,0},*/{42,0,49,8,0,1,1,0,0}};
/*[0]-индекс имени,[1]-длина параметра,[2]-адрес начальный
тип данных:строка-0,байт-1,одиночный выбор-2,множ.выбор-3,дейст.число-4
[3]-тип данных,[4]-номер для однотипных,иначе-128 [5]-индекс единиц,
  [6]-пароль для изменения нужен:1,2-изменение через MMI невозможно*/
unsigned char conf_main[Max_main][7]={
 {36/*Адрес станции(0)*/,1,1,1,128,0,1},{37/*Контракт час(1)*/,1,2,1,128,0,1},
 {38/*Терминал опер(2)*/,1,3,2,128,0,1},{39/*Адрес MVS #1(3)*/,1,4,1,128,0,1},
 {40/*Адрес MVS #2(4)*/,1,5,1,128,0,1},{41/*Адрес MVS #3(5)*/,1,6,1,128,0,1},
 {42/*Адрес MVS #4(6)*/,1,7,1,128,0,1},{45/*Адрес ICP #1(7)*/,1,8,1,128,0,1},
 {46/*Адрес ICP #2(8)*/,1,9,1,128,0,1},{47/*Адрес ICP #3(9)*/,1,10,1,128,0,1},
 {48/*Адрес ICP #4(10)*/,1,11,1,128,0,1},{43/*Летнее/зимнее(11)*/,1,12,3,128,0,2},
 {62/*HART(12)*/,1,13,2,128,0,2},{52/*Тип ICP #1(13)*/,1,14,2,128,0,1},
 {53/*Тип ICP #2(14)*/,1,15,2,128,0,1},{54/*Тип ICP #3(15)*/,1,16,2,128,0,1},
 {55/*Тип ICP #4(16)*/,1,17,2,128,0,1},{58/*Модбас(17)*/,1,18,7,128,0,2},
 {49/*Пароль(18)*/,4,19,5,128,0,2},{59/*Порт_1 (19)*/,2,23,6,0,0,2},
 {59/*Порт_2(20)*/,2,25,6,1,0,2},{59/*Порт_3 (21)*/,2,27,6,2,0,2},
 {59/*Порт_4(22)*/,2,29,6,3,0,2},{60/*Арх_1 (23)*/,4,31,8,0,0,2},
 {60/*Арх_2(24)*/,4,35,8,1,0,2},{60 /*Арх_3 (25)*/,4,39,8,2,0,2},
 {60/*Арх_4(26)*/,4,43,8,3,0,2},{60 /*Арх_5 (27)*/,4,47,8,4,0,2},
 {60/*Арх_6(28)*/,4,51,8,5,0,2},{60 /*Арх_7 (29)*/,4,55,8,6,0,2},
 {60/*Арх_8 (30)*/,4,59,8,7,0,2},{60 /*Арх_9 (31)*/,4,63,8,8,0,2},
 {60/*Арх_10 (32)*/,4,67,8,9,0,2},{60 /*Арх_11 (33)*/,4,71,8,10,0,2},
 {60/*Арх_12 (34)*/,4,75,8,11,0,2},{60 /*Арх_13 (35)*/,4,79,8,12,0,2},
 {60/*Арх_14 (63)*/,4,83,8,13,0,2},{60 /*Арх_15 (37)*/,4,87,8,14,0,2},
 {60/*Арх_16 (38)*/,4,91,8,15,0,2},{60 /*Арх_17 (39)*/,4,95,8,16,0,2},
 {60/*Арх_18 (40)*/,4,99,8,17,0,2},{60 /*Арх_19 (41)*/,4,103,8,18,0,2},
 {60/*Арх_20 (42)*/,4,107,8,19,0,2},{60 /*Арх_21 (43)*/,4,111,8,20,0,2},
 {60/*Арх_22 (44)*/,4,115,8,21,0,2},{60 /*Арх_23 (45)*/,4,119,8,22,0,2},
 {60/*Арх_24 (46)*/,4,123,8,23,0,2},{60 /*Арх_25 (47)*/,4,127,8,24,0,2},
 {60/*Арх_26 (48)*/,4,131,8,25,0,2},{60 /*Арх_27 (49)*/,4,135,8,26,0,2},
 {60/*Арх_28 (50)*/,4,139,8,27,0,2},{60 /*Арх_29 (51)*/,4,143,8,28,0,2},
 {60/*Арх_30 (52)*/,4,147,8,29,0,2},{60 /*Арх_31 (53)*/,4,151,8,30,0,2},
 {60/*Арх_32 (54)*/,4,155,8,31,0,2},{60 /*Арх_33 (55)*/,4,159,8,32,0,2},
 {60/*Арх_34 (56)*/,4,163,8,33,0,2},{60 /*Арх_35 (57)*/,4,167,8,34,0,2},
 {60/*Арх_36 (58)*/,4,171,8,35,0,2},{60 /*Арх_37 (59)*/,4,175,8,36,0,2},
 {60/*Арх_38 (60)*/,4,179,8,37,0,2},{60 /*Арх_39 (61)*/,4,183,8,38,0,2},
 {60/*Арх_40 (62)*/,4,187,8,39,0,2},{35 /*Тег (63)*/,10,191,10,128,0,2},
 {50/*Телеф.номер(64)*/,20,201,0,128,0,2},{51/*Модем(65)*/,10,221,10,128,0,2},
 {44/*Пароль операт(66)*/,2,231,9,128,0,1},
 {72/*Способ настр.порта(67)*/,1,233,2,128,0,1},
 {74/*Обработка скрипта(68)*/,1,234,2,128,0,1},
};
unsigned char main_select[Max_select_main]={
  10,2,3,0,51,52,  11,2,53,54,  13,7,55,56,57,58,35,36,114,
  14,7,55,56,57,58,35,36,114,  15,7,55,56,57,58,35,36,114,
  16,7,55,56,57,58,35,36,114, 12,2,0,51, 67,2,87,88, 68,2,0,1, 69,2,0,1};
/*[0]-индекс имени,[1]-длина параметра,[2]-адрес начальный
тип данных:строка-0,байт-1,одиночный выбор-2,множ.выбор-3,дейст.число-4
[3]-тип данных,[4]-номер для однотипных,иначе-128 [5]-индекс единиц,
  [6]-пароль для изменения нужен:1,2-изменение через MMI невозможно*/
unsigned char conf_basic[Max_conf][7]={ //20.09.2021 trim
 {1/*Статус точки(0)*/,1,0,2,128,0,1},{2/*Выбор сенсора(1)*/,1,1,2,128,0,1},
 {3/*Выбор р.расх.(2)*/,1,2,2,128,0,1},{4/*Выбор давл(3)*/,1,3,2,128,0,1},
 {5/*Выбор темп(4)*/,1,4,2,128,0,1},{6/*Выбор доппереп(5)*/,1,5,2,128,0,1},
 {61/*Выбор термокор(6)*/,1,6,2,128,0,1},{56/*Материал трубы(7)*/,1,7,2,128,0,1},
 {57/*Материал сенсора(8)*/,1,8,2,128,0,1},{64/*Метод расчёта(9)*/,1,9,2,128,0,1},
 {75/*Перкл кромки(10)*/,1,10,2,128,0,1},
 {9/*Межповер инт(11)*/,4,11,4,128,22,1},
 {11/*Диаметр трубы(12)*/,4,15,4,128,1,1},{12/*Диам сенсора(13)*/,4,19,4,128,1,1},
 {13/*Отсечка (14)*/,4,23,4,128,2,1},//{16/*Радиус кромки(15)*/,4,27,4,128,1,1},
 //{17/*Шероховатость(16)*/,4,31,4,128,1,1},
 {69/*Плотность (17)*/,4,35,4,128,8,1},{70/*Азот  (18)*/,4,39,4,128,20,1},
 {71/*Углек.газ(19)*/,4,43,4,128,20,1},//{18/*КТР А трубы(20)*/,4,47,4,128,11,1},
 //{19/*КТР В трубы (21)*/,4,51,4,128,11,1},{20/*КТР С трубы(22)*/,4,55,4,128,11,1},
 //{21/*КТР А сенсора(23)*/,4,59,4,128,11,1},{22/*КТР В сенсора(24)*/,4,63,4,128,11,1},
 //{23/*КТР С сенсора(25)*/,4,67,4,128,11,1},
 //{63/*Корр мин.расх(26)*/,1,71,2,128,0,2},{7/*Статус алармов(27)*/,1,72,3,128,0,2},
 //{14/*Верхняя граница(28)*/,4,73,4,128,2,1},{15/*Нижняя гран(29)*/,4,77,4,128,2,1},
 //{26/*Ниж алм пер(30)*/,4,81,4,128,2,1},
 //{27/*Верх алм перепад(31)*/,4,85,4,128,2,1},{28/*Гист перепад(32)*/,4,89,4,128,2,1},
 //{29/*Ниж алм давл(33)*/,4,93,4,128,2,1},{30/*Верх алм давл(37)*/,4,97,4,128,2,1},
 //{31/*Гистер давл(35)*/,4,101,4,128,2,1},{32/*Ниж алм темп(36)*/,4,105,4,128,3,1},
// {33/*Верх алм темп(37)*/,4,109,4,128,3,1},{34/*Гистерез тем(38)*/,4,113,4,128,3,1},
 //{24/*Миним расход(39)*/,4,117,4,128,17,1},{25/*Макс расход (40)*/,4,121,4,128,17,1},
 /*{76/*Гистер расход (41)*//*,4,125,4,128,17,1},*/{35/*Тег идент(42)*/,10,129,10,128,0,2}
};
unsigned char conf_select[Max_select]={
  13,0,2,0,1, 1,12,2,3,4,5,6,7,8,9,10,11,12,13,
  2,21,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
  3,21,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
  4,21,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
  5,21,0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
  6,2,59,60, 27,4,45,46,47,48, 26,2,0,1,
  7,11,98,62,63,99,100,66,101,68,102,81,103,
  8,11,72,80,104,81,103,105,106,107,108,109,110, 9,3,84,85,86, 10,2,96,97};
/*[0]-единицы измерения
  [1]-нач адрес параметра в списке точки
  [2]-тип архивирования
  [3]-имя параметра*/
/*перепад,давление,температура,плотность,азот,угл.газ,расход*/
const unsigned char set_archive[Max_archive][4]={
  {2,0,3,1},{2,4,3,2},{3,8,3,3},{8,12,3,4},{20,16,3,12},
  {20,20,3,13},{18,24,2,48}};
unsigned char conf_icp[Max_icp_prm][7]={
  {65,1,0,12,0,0,2},{65,1,1,12,1,0,2},{65,1,2,12,2,0,2},{65,1,3,12,3,0,2},
  {65,1,4,12,4,0,2},{65,1,5,12,5,0,2},{65,1,6,12,6,0,2},{65,1,7,12,7,0,2},
  {65,1,8,12,8,0,2},{65,1,9,12,9,0,2},{65,1,10,12,10,0,2},
  {65,1,11,12,11,0,2},{65,1,12,12,12,0,2},{65,1,13,12,13,0,2},
  {65,1,14,12,14,0,2},{65,1,15,12,15,0,2},{66,4,16,4,0,0,1},
  {66,4,20,4,1,0,1},{66,4,24,4,2,0,1},{66,4,28,4,3,0,1},{66,4,32,4,4,0,1},
  {66,4,36,4,5,0,1},{66,4,40,4,6,0,1},{66,4,44,4,7,0,1},{66,4,48,4,8,0,1},
  {66,4,52,4,9,0,1},{66,4,56,4,10,0,1},{66,4,60,4,11,0,1},{66,4,64,4,12,0,1},
  {66,4,68,4,13,0,1},{66,4,72,4,14,0,1},{66,4,76,4,15,0,1},{67,4,80,4,0,0,1},
  {67,4,84,4,1,0,1},{67,4,88,4,2,0,1},{67,4,92,4,3,0,1},{67,4,96,4,4,0,1},
  {67,4,100,4,5,0,1},{67,4,104,4,6,0,1},{67,4,108,4,7,0,1},{67,4,112,4,8,0,1},
  {67,4,116,4,9,0,1},{67,4,120,4,10,0,1},{67,4,124,4,11,0,1},
  {67,4,128,4,12,0,1},{67,4,132,4,13,0,1},{67,4,136,4,14,0,1},{67,4,140,4,15,0,1},
  {77,1,144,12,0,0,2},{77,1,145,12,1,0,2},{77,1,146,12,2,0,2},{77,1,147,12,3,0,2},
  {77,1,148,12,4,0,2},{77,1,149,12,5,0,2},{77,1,150,12,6,0,2},{77,1,151,12,7,0,2},
  {78,4,152,4,0,0,1},{78,4,156,4,1,0,1},{78,4,160,4,2,0,1},{78,4,164,4,3,0,1},
  {78,4,168,4,4,0,1},{78,4,172,4,5,0,1},{78,4,176,4,6,0,1},{78,4,180,4,7,0,1},
  {79,4,184,4,0,0,1},{79,4,188,4,1,0,1},{79,4,192,4,2,0,1},{79,4,196,4,3,0,1},
  {79,4,200,4,4,0,1},{79,4,204,4,5,0,1},{79,4,208,4,6,0,1},{79,4,212,4,7,0,1},
  {99,4,216,4,0,0,1},{99,4,220,4,1,0,1},
};
unsigned char conf_exp[Max_exp_prm][5]={
 {68,4,0,14,0},{68,4,4,14,1},{68,4,8,14,2},{68,4,12,14,3},
 {68,4,16,14,4},{68,4,20,14,5},{68,4,24,14,6},{68,4,28,14,7},
 {68,4,32,14,8},{68,4,36,14,9},{68,4,40,14,10},{68,4,44,14,11},
 {68,4,48,14,12},{68,4,52,14,13},{68,4,56,14,14},{68,4,60,14,15},
 {68,4,64,14,16},{68,4,68,14,17},{68,4,72,14,18},{68,4,76,14,19},
 {68,4,80,14,20},{68,4,84,14,21},{68,4,88,14,22},{68,4,92,14,23},
 {68,4,96,14,24},{68,4,100,14,25},{68,4,104,14,26},{68,4,108,14,27},
 {68,4,112,14,28},{68,4,116,14,29},{68,4,120,14,30},{68,4,124,14,31},
 {68,4,128,14,32},{68,4,132,14,33},{68,4,136,14,34},{68,4,140,14,35},
 {68,4,144,14,36},{68,4,148,14,37},{68,4,152,14,38},{68,4,156,14,39},
 {73,4,160,4,0},{73,4,164,4,1},{73,4,168,4,2},{73,4,172,4,3},{73,4,176,4,4},
 {73,4,180,4,5},{73,4,184,4,6},{73,4,188,4,7},{73,4,192,4,8},{73,4,196,4,9},
 {73,4,200,4,10},{73,4,204,4,11},{73,4,208,4,12},{73,4,212,4,13},
 {73,4,216,4,14},{73,4,220,4,15},{73,4,224,4,16},{73,4,228,4,17},
 {73,4,232,4,18},{73,4,236,4,19}};

struct station
{
  unsigned char task;        /*тип решаемой задачи*/
  unsigned char addr;        /*адрес контроллера*/
  unsigned char contrh;       /*контрактный час*/
  unsigned char mmi;         /*применение терминала оператора*/
  unsigned char adr_mvs[4];  /*адреса сенсоров*/
  unsigned char mvs_eep[4];  /*адрес ячейки в EEPROM*/
  unsigned char adr_icp[4];  /*адреса модулей*/
  unsigned char autogo;      /*автопереход на летнее/зимнее время*/
  unsigned char hart;
  unsigned char typ_icp[4];              /*тип модуля*/
  unsigned char modbus;
  unsigned char passw[4];               /*пароль на изменение конфигурации*/
  unsigned char com[4][2];               /*настройки портов*/
  unsigned char arch[Max_arch_pnt][4];   /*настройки архивных точек*/
  unsigned char tag[10];
  unsigned char phone[20];
  unsigned char at[10];
  unsigned char set_com;                 /*разрешение на СОМ4*/
  unsigned      passw_op;                /*пароль оператора*/
  unsigned char script;
  unsigned char units;
  unsigned char panel;
};

struct configparam
{
  unsigned char status;       /*точка включена или выключена*/
  unsigned char sensor;       /*выбор сенсора*/
  unsigned char s_diff;       /*выбор источника перепада*/
  unsigned char s_press;      /*выбор источника давления*/
  unsigned char s_tempr;      /*выбор источника температ*/
  unsigned char stack;        /*выбор источника доп.перепада*/
  unsigned char enable_alm;   /*статус алармов*/
  unsigned char therm_corr;    /*выбор термокоррекции*/
  unsigned char flow_corr;    /*включение коррекции расхода*/
  unsigned char m_pipe;       /*материал трубы*/
  unsigned char m_sens;
  unsigned char method;
  unsigned char cur_sens;     /*текущий перепадчик*/
  unsigned char s_egle;
  float         interv;       /*мнжповер.интерв*/
  float         pipe_size;    /*диаметр трубы*/
  float         sens_size;    /*диаметр сенсора*/
  float         cut_flow;     /*отсечка мин.расхода*/
  float         hi_stack;     /**/
  float         lo_stack;     /**/
  float         radius;       /*радиус кромки диафр*/
  float         rougness;     /*шероховатость трубы*/
  float         k_pipe[3];      /*коэффициенты терморасширения*/
  float         k_sens[3];
  float         borders[4][3];
  unsigned char tag[10];
  unsigned char flag;         /*флаг рекалькуляции*/
  unsigned char status_alm[4];/*текущее состояние алармов*/
};

/******** установка или очистка алармов параметра****************/
unsigned char SetClearAlarmsPrm (float borders[],unsigned char status,
				 float value,unsigned char num_pnt,
				 unsigned char num_prm)
{ /*статус: 0-нет алармов, 1-нижний, 2-верхний*/
  /*начальный адрес статусов алармов в ЭНП : 0С320 */
  const unsigned char num_alm[4][2]={{0,1},{2,3},{4,5},{6,7}};
  unsigned char flag,cr[4],j,buf_alm[16];
  if (borders[2]<=0.0 || (borders[1]-borders[0])<=0.0) 
  {
    flag=2;
    goto M;
  }
  FormateEvent(buf_alm);
  buf_alm[13]=num_pnt;
  flag=0;
  if (status == 0  && value > borders[1])
  {
    status=2;
    flag=1;
    buf_alm[10]=1;
    buf_alm[14]=num_alm[num_prm][1];
  } 
  else if (status == 2 && value<=(borders[1]-borders[2]))
  {
    status=0;
    flag=1;
    buf_alm[10]=0;
    buf_alm[14]=num_alm[num_prm][1];
  } 
  else if (status == 0 && value < borders[0])
  {
    status=1;
    flag=1;
    buf_alm[10]=1;
    buf_alm[14]=num_alm[num_prm][0];
  } 
  else if (status == 1 && value>= (borders[0]+borders[2]))
  {
    status=0;
    flag=1;
    buf_alm[10]=0;
    buf_alm[14]=num_alm[num_prm][0];
  }
  if (flag == 1)
  {
    ConvToBynare(value,cr);
    for (j=0;j<4;j++) buf_alm[6+j]=cr[j];
    WriteEvent(buf_alm,1);
    X607_Write(390,32+num_pnt*4+num_prm,status);
  }
  M:  return status;
}

/******** установка или очистка алармов точки учёта ***************/
void  SetClearAlarms (struct configparam *cnf,struct dynparams bs,
		      unsigned char num_pnt)
{
  unsigned char i;
  for (i=0;i<3;i++) if ((cnf->enable_alm & etl[i]) > 0)
    cnf->status_alm[i]=SetClearAlarmsPrm(cnf->borders[i],cnf->status_alm[i],
		  bs.dyn[main_dyn[i][0]],num_pnt,i);
  if ((cnf->enable_alm & etl[3]) > 0) /*для массового расхода*/
    cnf->status_alm[3]=SetClearAlarmsPrm(cnf->borders[3],cnf->status_alm[3],
		  bs.dyn[main_dyn[6][0]],num_pnt,3);
}

/******* инициализация параметров структуры: станция ***********/
void InitStationStruct (struct station *dv)
{
  unsigned char i;
  dv->task=ReadEEP(7,0);
  dv->addr=ReadEEP(7,1);
  dv->contrh=ReadEEP(7,2);
  dv->mmi=ReadEEP(7,3);
  for (i=0;i<4;i++) dv->adr_mvs[i]=ReadEEP(7,4+i);
  for (i=0;i<4;i++) dv->mvs_eep[i]=4+i;
  for (i=0;i<4;i++) dv->adr_icp[i]=ReadEEP(7,8+i);
  for (i=0;i<4;i++) dv->typ_icp[i]=ReadEEP(7,14+i);
  for (i=0;i<4;i++) dv->passw[i]=ReadEEP(7,19+i);
  dv->autogo=ReadEEP(7,12);
  dv->hart=ReadEEP(7,13);
  dv->modbus=ReadEEP(7,18);
  for (i=0;i<4;i++)
  {
    dv->com[i][0]=ReadEEP(7,23+i*2);
    dv->com[i][1]=ReadEEP(7,24+i*2);
  }
  for (i=0;i<10;i++) dv->tag[i]=ReadEEP(7,191+i);
  for (i=0;i<20;i++) dv->phone[i]=ReadEEP(7,201+i);
  for (i=0;i<10;i++) dv->at[i]=ReadEEP(7,221+i);
  Modbus.mode=(dv->modbus & 16) >> 4;
  Modbus.delay=dv->modbus & 7;
  Modbus.protocol=(dv->modbus & 8) >> 3;
  Modbus.connect=(dv->modbus & 96) >> 5;
  dv->set_com=ReadEEP(7,233);
  dv->passw_op=ReadEEP(7,231)*256+ReadEEP(7,232);
  dv->script=ReadEEP(7,234);dv->panel=ReadEEP(7,235);
}

/******* инициализация структуры:конфигурация учёта газа**********/
void InitBasicStruct (unsigned char num,struct configparam *cnf,
		      struct dynparams *bs)
{
  unsigned char i,j,k,cr[4];
  float buf[16];
  cnf->status=ReadEEP(num,0);
  cnf->sensor=ReadEEP(num,1);
  cnf->s_diff=ReadEEP(num,2);
  cnf->s_press=ReadEEP(num,3);
  cnf->s_tempr=ReadEEP(num,4);
  cnf->stack=ReadEEP(num,5);
  cnf->enable_alm=ReadEEP(num,72);
  cnf->therm_corr=ReadEEP(num,6);
  cnf->flow_corr=ReadEEP(num,71);
  cnf->m_pipe=ReadEEP(num,7);
  cnf->m_sens=ReadEEP(num,8);
  cnf->method=ReadEEP(num,9);
  cnf->s_egle=ReadEEP(num,10);
  for (i=0;i<15;i++)
  { /*получение основных конфиг.параметров*/
    j=i*4;
    if (SecurityConvert(ReadEEP(num,11+j),ReadEEP(num,12+j),
      ReadEEP(num,13+j),ReadEEP(num,14+j),cr)==0)
        buf[i]=ConvToFloat(cr[0],cr[1],cr[2],cr[3]);
  } 
  cnf->interv=buf[0];
  cnf->pipe_size=buf[1];
  cnf->sens_size=buf[2];
  cnf->cut_flow=buf[3];
  cnf->radius=buf[4];
  cnf->rougness=buf[5];
  bs->dyn[12]=buf[6];
  bs->dyn[16]=buf[7];
  bs->dyn[20]=buf[8];
  for (k=0;k<3;k++) 
  {
    cnf->k_pipe[k]=buf[9+k];
    cnf->k_sens[k]=buf[12+k];
  }
  for (i=0;i<14;i++)
  { /*получение уставок алармов*/
    j=i*4;
    if (SecurityConvert(ReadEEP(num,73+j),ReadEEP(num,74+j),
      ReadEEP(num,75+j),ReadEEP(num,76+j),cr)==0)
        buf[i]=ConvToFloat(cr[0],cr[1],cr[2],cr[3]);
  } 
  cnf->hi_stack=buf[0];
  cnf->lo_stack=buf[1];
  for (i=0;i<4;i++) for (j=0;j<3;j++) cnf->borders[i][j]=buf[2+3*i+j];
  for (i=0;i<10;i++) cnf->tag[i]=ReadEEP(num,129+i);
}

/******* инициализация параметров архива *************/
unsigned char InitArchive (struct station *dv)
{
  unsigned char i,k;k=0;
  for (i=0;i<Max_arch_pnt;i++)
  {
    dv->arch[i][0]=ReadEEP(7,31+i*4);
    dv->arch[i][1]=ReadEEP(7,32+i*4);
    dv->arch[i][2]=ReadEEP(7,33+i*4);
    dv->arch[i][3]=ReadEEP(7,34+i*4);
    if (dv->arch[i][0] != 0) k++;
  } 
  return 7+k*4;
}

/******** проверка ширины аннубара и коррекция если +- 10% *******/
unsigned char VerifyAnnubar (unsigned char tp,float *annubar)
{
  const float d[8] = {4.3942,9.271,21.7424,31.369,49.53,14.986,
		26.924,48.768};/*средняя ширина аннубара*/
  if ((*annubar > 1.1 * d[tp])||(*annubar < 0.9 * d[tp]))
  { 
    *annubar=d[tp];
    return 1;
  } 
  else return 0;
}

/******** вычисление коэффициента блокировки трубы ***************/
double BettaKoeff (float pipe,float annubar)
{
  return 1.273239 * annubar / pipe;
}

/********* вычисление коэффициента потока по типу аннубара *******/
double AnnubarKoeff (double betta,unsigned char type)
{
  const double c1[8] = {-0.8212,-1.3452,-1.43,-1.3416,-1.2957,-1.515,
		-1.492,-1.5856};
  const double c2[8] = {0.7269,0.92,1.265,1.2075,1.24,1.4229,1.4179,
		1.3318};
  double a,c,d; 
  a=0.0;
  if (type == 0) a = c1[type] * betta + c2[type];
  else if (type < 8)
  {
    d = 1 - c2[type] * betta;c = d * d;
    c = sqrt(1 - c1[type] * c);
    a = d / c;
  }
  return a;
 }

/*** вычисление коэфф.потока по типу диафрагмы:фланцевая,углов ***/
float CalcOrifNewC (float rein,float betta,float pipe,float b4,
		    unsigned char type)
{ /*диаметр трубы в метрах*/
  double A,M1,M2,L1,L2;
  if (rein>1000)
  {
    switch (type)
    {
      case 0:L1=L2=0;break; /*угловой*/
      case 1:L1=L2=0.0254/pipe;break;/*фланцевый*/
      case 2:L1=1;L2=0.47;break;/*трёхрадиусный*/
    } 
    A=pow(19000*betta/rein,0.8);
    M1=2*L2/(1-betta);
    if (pipe<0.07112) M2=0.011*(0.75-betta)*(2.8-pipe/0.0254);
    else M2=0;
    return 0.5961+0.0261*betta*betta-0.216*b4*b4+
	    0.000521*pow(1E6*betta/rein,0.7)+(0.0188+0.0063*A)*pow(betta,3.5)*
	      pow(1E6/rein,0.3)+(0.043+0.08*exp(-10*L1)-0.123*exp(-7*L1))*
	        (1-0.11*A)*b4/(1-b4)-0.031*(M1-0.8*pow(M1,1.1))*pow(betta,1.3)+M2;
  } 
  else return 1.0;
}

/********* вычисление поправки на притупление кромки**************/
/** диаметр диафр. и радиус в м,межповерочный интерв.в годах ****/
float CalcOrifNewEgle (double radius,float sensor,float interv,
		       unsigned char stack)
{
  double rk;radius=radius/1E3;
  if (stack == 0 && interv > 0)
    rk=0.000195-(3.0/interv)*(0.000195-radius)*(1-exp(-interv/3.0));
  if (stack == 1) rk=0.000195-(0.000195-radius)*exp(-interv/3.0);
  if (rk <= 0.0004*sensor) return 1.0;
  else return 0.9826+pow((rk/sensor+0.0007773),0.6);
}

/************ вычисление коэфф.расширения на диафрагме **********/
float CalcOrifNewExpand (float diff,float press,double b4,float adiab)
{ /*перепад и давление в паскалях*/
  return 1-(0.351+0.256*b4+0.93*b4*b4)*(1-pow(1-diff/press,1/adiab));
}

/************** выполняет округление до целого ******************/
float RoundFloat (double value)
{
  float a,b;a=fabs(value);
  b=floor(a);
  if ((a-b)>=0.5) b=b+1;
  if (value < 0) b=-b;
  return b;
}

/************** округляет до двух старших разрядов **************/
float RoundToTwo (double value)
{
  int a;
  a=log10(value);
  if (a>1) value=RoundFloat(value/pow(10,a-1))*pow(10,a-1); 
  else if (a>-2) value=RoundFloat(value*10)/10;
  else value=0.0; 
  return value;
}

/********* вычисляет поправку на шероховатость трубы ************/
float CalcOrifNewRougness (float rein,float betta,double pipe,
			   double rougness)
{ /*все размеры в метрах*/
  const double kf1[3][4]={{8.87,-3.7114,0.41841,0.0},
		{6.7307,-5.5844,0.732485,0.0},
			{-10.244,5.7094,-0.76477,0.0}};
  const double kf2[3][4]={{27.23,-11.458,1.6117,-0.07567},
		{-25.928,12.426,-2.09397,0.106143},
			{1.7622,-3.8765,1.05567,-0.076764}};
  const double kf3[3][4]={{16.5416,-6.60709,0.88147,-0.039226},
		{322.594,-132.2,17.795,-0.799765},
			{-92.029,37.935,-5.1885,0.23583}};
  const float pi=3.141592;
  double max,min,logrein,lj,lj1,kr,kd,Ash,A[3],avg;
  unsigned char i,k;
  if (rein > 1000)
  {
    logrein=log10(rein);
    rougness=rougness/1E3;
    for (i=0;i<3;i++) A[i]=0.0;
    avg=rougness/pi;
    if (rein>1E4 && rein<=1E5) for (i=0;i<3;i++) for (k=0;k<4;k++)
      A[i]=A[i]+kf1[i][k]*pow(logrein,k); 
    else if (rein>1E5 && rein<=3E6) for (i=0;i<3;i++) for (k=0;k<4;k++)
      A[i]=A[i]+kf2[i][k]*pow(logrein,k); 
    else if (rein>3E6 && rein<=1E8) for (i=0;i<3;i++) for (k=0;k<4;k++)
      A[i]=A[i]+kf3[i][k]*pow(logrein,k);
    if (rein<=1E4) max=0.718866*pow(betta,-3.887)+0.364; 
    else if (rein>1E4 && betta<0.65) max=A[0]*pow(betta,A[1])+A[2]; 
    else if (rein>1E4 && betta>=0.65) max=A[0]*pow(0.65,A[1])+A[2];
    if (max>=15) max=15*pipe/1E4;
    else 
    {
      max=RoundToTwo(max);
      max=max*pipe/1E4;
    }
    if (rein < 3E6) min=0.0;
    else
    {
      if (betta<0.65) min=7.1592-12.387*betta-(2.0118-3.469*betta)*logrein+
		    (0.1382-0.23762*betta)*logrein*logrein; 
      else
        min=-0.892353+0.24308*logrein-0.0162562*logrein*logrein;
      if (min <= 0) min=0;
      else 
      {
        min=RoundFloat(min*1E3)/1E3;
        min=min*pipe/1E4;
      }
    }
    if (avg > max || avg < min)
    {
      Ash=rougness;
      kd=0.26954*rougness/pipe;
      kr=5.035/rein;
      lj=1.74-2*log10(2*Ash/pipe-37.36*log10(kd-kr*log10(kd+3.3333*kr))/rein);
      lj=pow(lj,-2.0);
      if (avg>max) 
      {
        Ash=pi*max;
        kd=0.846784*max/pipe;
      }
      if (avg<min) 
      {
        Ash=pi*min;
        kd=0.846784*min/pipe;
      }
      lj1=1.74-2*log10(2*Ash/pipe-37.36*log10(kd-kr*log10(kd+3.3333*kr))/rein);
      lj1=pow(lj1,-2.0);
      return 1+5.22*pow(betta,3.5)*(lj-lj1);
    } 
    else return 1.0;
  } 
  else return 1.0;
}

/************** вычисление показателя адиабаты ******************/
float CalcAdiabat (float abstemp,float pmega,float azot,float dens)
{
  return 1.556 * (1 + 0.074 * azot) - 3.9E-4 * abstemp * (1 - 0.68 * azot)
    - 0.208 * dens + pow(pmega/abstemp,1.43) * (384 * (1 - azot) *
      pow(pmega/abstemp,0.8) + 26.4 * azot);
}

/************ вычисление коэффициента расширения ****************/
float YaKoeff (float betta,float diff,float press,float adiabat)
{
  double expan;
  expan = (1 - betta) * (1 - betta) * 0.31424 - 0.09484;
  if (press > 0)
    return (1 - expan * diff / press / adiabat);
  else return 1.0;
}

/*** температурная коррекция диаметра трубы по типу материала ****/
double DiameterCorrectP(float diametr,float temper,unsigned char type)
{
  const float ka[11] = {10.821,10.8,11.1,11.66,10.75,11.1,10.26,10.2,
		11.6,10.0,16.206};
  const float kb[11] = {17.872,9.0,7.9,9.0,12.5,7.7,14.0,10.4,0.0,9.6,6.571};
  const float kc[11] = {-10.986,-4.2,-3.9,0.0,0.0,-3.4,0.0,-5.6,0.0,-6.0,0.0};
  double a;
  a = 1.0E-6*(ka[type]+1.0E-3*temper*kb[type]-1.0E-6*temper*temper*kc[type]);
  return diametr*(1+a*(temper-20));
}

/************* температурная коррекция диаметра СУ ***************/
double DiameterCorrectD(float diametr,float temper,unsigned char type)
{
  const float ka[11] = {15.2,14.7,16.466,10.0,16.206,10.235,11.065,15.6,
		15.22,16.216,15.8};
  const float kb[11] = {6.6,9.8,5.36,9.6,6.571,18.64,11.224,8.3,13.0,6.4,0.0};
  const float kc[11] = {0.6,7.4,3.0,-6.0,0.0,-13.0,-5.381,-6.5,0.0,0.0,0.0};
  double a;
  a = 1.0E-6*(ka[type]+1.0E-3*temper*kb[type]-1.0E-6*temper*temper*kc[type]);
  return diametr*(1+a*(temper-20));
}

/*********** вычисление коэффициента сжимаемости при станд.усл.*****/
double CalcSFactor (float density,float N2,float CO2)
{
  double factor;
  factor = 0.0741 * density - 0.006 - 0.063 * N2 - 0.0575 * CO2;
  factor = factor * factor;return (1 - factor);
}

/*****************************************************************/
double CalcPpk (float density,float N2,float CO2)
{
  return 2.9585 *(1.608 - 0.05994*density - 0.392 * N2 + CO2);
}

/*****************************************************************/
double CalcTpk (float density,float N2,float CO2)
{
  return 88.25 *(0.9915 + 1.759*density - 1.681 * N2 - CO2);
}

/********* вычисление коэффициента сжимаемости по NX-19 **********/
/* давление в Мпа,температура в гр.Кельвина */
unsigned char CalcNX19Koeff (double Ppk,double Tpk,float press,
		   float temper,float standart,float *compress)
{
  double Pa,Ta,Ta2,Ta4,Pa2,musor,factor;
  unsigned char type;
  double Q0,Q1,Q02,B0,B1,B2,dTa,F,Pa23,dTa05,dTa2,E20dTa;

  Pa = 0.6714 * press / Ppk + 0.0147;
  Ta = 0.71892 * temper / Tpk + 0.0007;
  type=0;dTa =Ta - 1.09;
  if (Pa >= 0) if (Pa <= 2) if (dTa >= 0) if (dTa <= 0.3) type=1;
  if (Pa >= 0) if (Pa < 1.3) if (dTa >= -0.25) if (dTa < 0) type=2;
  if (Pa >= 1.3) if (Pa < 2) if (dTa >= -0.25) if (dTa < 0) type=3;
  if (type != 0)
  {
    Pa2 = Pa * Pa;Ta2 = Ta * Ta;Ta4 = Ta2 * Ta2;
    Q1 = Ta4 * Ta / (Ta2 * (6.60756 * Ta - 4.42646) + 3.22706);
    Q0 = Ta2 * (1.77218 - 0.8879 * Ta) + 0.30513;
    Q0 = Q0 * Q1 / Ta4;Q02 = Q0 * Q0;
    B1 = 2 * Q1 / 3 - Q02;dTa2 = dTa * dTa;
    Pa23 = 0.00075 * pow(Pa,2.3);
    E20dTa = exp(20 * dTa);
    switch (type)
    {
      case 1:
        dTa05 = sqrt(dTa);musor = Pa * (2.17 - Pa +1.4 * dTa05);
	      F = Pa23 / E20dTa + 0.0011 * dTa05 * musor * musor;
      break;
      case 2:
        F = Pa23 * (2 - E20dTa) + 1.317 * Pa * (1.69 - Pa2) * dTa2 *
		      dTa2;
      break;
      case 3:
        F = Pa23 * (2 - E20dTa) + 0.455 * (1.3 - Pa) * (4.01952 - Pa2) *
		      (dTa *(0.03249 + 18.028 * dTa2) + dTa2 *(2.0167 + dTa2 *
		        (42.844 + 200 * dTa2)));
      break;
    }
    B0 = Q0 * (Q1 - Q02) + 0.1 * Q1 * Pa * (F - 1);
    B2 = B0 + sqrt(B0 * B0 + B1 * B1 * B1);
    B2 = pow(B2,0.3333333);
    factor = 1 + 0.00132 / pow(Ta,3.25); 
    factor = factor * factor;
    factor = factor * Pa * 0.1 / (B1 / B2 - B2 + Q0);
    *compress=factor / standart;
    return 0;
  } 
  else 
  {
    *compress=1.0;
    return 1;
  }
}

/********* вычисление коэффициента сжимаемости по GERG-91 **********/
/* давление в Мпа,температура в гр.Кельвина,азот и уг.газ в моляр */
unsigned char CalcGERG91Koeff (float azt,float ugl,float press,
	   float temper,float standart,float dens,float *compress)
{
  double b,Xe,Xe2,Me,H,H2,T,T2,B1,B2,B23,B3,C1,C2,C3,Az2,Ug2;
  double C0,B0,A0,A1,A2,C223,C233,Bz,Cz,Bm,Cm,Z,aa,k;
  T=temper;
  b=1000*press/(2.7715*T);
  T2=T*T;
  Az2=azt*azt;
  Ug2=ugl*ugl;
  Xe=1-azt-ugl;
  k=0.3333333333;
  Me=(24.05525*standart*dens-28.0135*azt-44.01*ugl)/Xe;Xe2=Xe*Xe;
  H=128.64+47.479*Me;H2=H*H;
  B1=-0.425468+2.865*T/1000-4.62073*T2/1E6+
    (8.77118/10000-5.56281*T/1E6+8.8151*T2/1E9)*H+
      (-8.24747/1E7+4.31436*T/1E9-6.08319*T2/1E12)*H2;
  B2=-0.1446+7.4091*T/10000-9.1195*T2/1E7;
  B23=-0.339693+1.61176*T/1000-2.04429*T2/1E6;
  B3=-0.86834+4.0376*T/1000-5.1657*T2/1E6;
  C1=-0.302488+1.95861*T/1000-3.16303*T2/1E6+
    (6.46422/10000-4.22876*T/1E6+6.88157*T2/1E9)*H+
      (-3.32805/1E7+2.2316*T/1E9-3.67713*T2/1E12)*H2;
  C2=7.8498/1000-3.9895*T/1E5+6.1187*T2/1E8;
  C3=2.0513/1000+3.4888*T/1E5-8.3703*T2/1E8;
  C223=5.52066/1000-1.68609*T/1E5+1.57169*T2/1E8;
  C233=3.58783/1000+8.06674*T/1E6-3.25798*T2/1E8;
  Bz=0.72+1.875/1E5*(320-T)*(320-T);
  Cz=0.92+0.0013*(T-270);
  Bm=Xe2*B1+Xe*azt*Bz*(B1+B2)-1.73*Xe*ugl*sqrt(B1*B3)+
    Az2*B2+2*azt*ugl*B23+Ug2*B3;
  Cm=Xe2*Xe*C1+3*Xe2*azt*Cz*pow(C1*C1*C2,k)+2.76*Xe2*ugl*pow(C1*C1*C3,k)+
     3*Xe*Az2*Cz*pow(C1*C2*C2,k)+6.6*Xe*azt*ugl*pow(C1*C2*C3,k)+
      2.76*Xe*Ug2*pow(C1*C3*C3,k)+Az2*azt*C2+3*Az2*ugl*C223+
        3*azt*Ug2*C233+Ug2*ugl*C3;
  C0=b*b*Cm;B0=b*Bm;A1=1+B0;A0=1+1.5*(B0+C0);aa=A0*A0-A1*A1*A1;
  if (aa>=0)
  {
    aa=A0-sqrt(aa);
    if (aa>=0)
    {
      A2=pow(aa,k);
      Z=(1+A2+A1/A2)/3.0;
      *compress=Z/standart;
      return 0;
    } 
    else  
    {
      *compress=1.0;
      return 1;
    }
  } 
  else 
  {
    *compress=1.0;
    return 1;
  }
}

/********* вычисление мгновенного расхода при станд. усл.**********/
//16.09.2021
float CalcFlowRate (float work_flow,float press,float temper,float Z)
{
  if( Z == 0 ) Z = 1.0;
  return (273.15 / 101.325) * work_flow * (1 / Z) * (press / temper); 
}

/********** вычисление динамической вязкости ********************/
float CalcViscosity (float temper,float density,float N2,float CO2)
{
  return 3.24 * (sqrt(temper) + 1.37 - 9.09 * pow(density,0.125)) /
	  (sqrt(density) + 2.08 - 1.5 * (N2 + CO2))/1E3;
}

/*********** вычисление динамической вязкости при повыш. давл.******/
float CalcViscExt (float Vs,float Ppk,float Tpk,float press,
		   float temper)
{
  press = press / Ppk;
  temper = temper / Tpk;
  return  Vs * (1 + press * press / 30 / (temper - 1));
}

/*********** вычисление плотности при раб. усл. в кг/м **********/
float CalcDensWork (float density,float press,float temper,float Z)
{
  if ((temper > 0)&&(Z > 0))
    return density * press * 2.893165 / temper / Z;
  else return 0.0;
}

/********** вычисление скорости потока в м/сек *******************/
float CalcVelosity (float density,float flow,float work,float pipe)
{
  if (work > 0)
    return 0.000353675 * flow * density / pipe / pipe / work;
  else return 0.0;
}

/************* вычисление числа Рейнольдса для аннубара **********/
float CalcReinAnnubar (float sens_m,float velosity,float density,
		       float visc)
{
  float value;
  if (visc > 0) value=sens_m*velosity*density/visc;
  else value=0;
  return value;
}

/*****************************************************************/
/******** вычисление всех параметров расхода *********************/
/* входные параметры:перепад(0),давление(4),температура(8),
  плотность(12),азот(16),углек.газ(20),мгнов.расход(24),коэфф.сжимаемости(32),
  коэфф.потока(33),коэфф.расширения(34),динам.вязкость(39),
  рабоч.плотность(42),скорость потока(40),число Рейнольдса(38),
  поправка на неостроту кромки(35),поправка на шероховатость(36),
  поправка на число Рейнольдса(37),показатель адиабаты(41) *******/
  //16.09.2021
unsigned char CalcFlowMain (struct configparam Prm,struct dynparams *Bs)
{
  float azt,ugl,std,Pp,Tp,tabs,pmega,Z;
  float work_flow;
  double pipe_m;
  unsigned char result, flag; 

  result=flag=0;
  Bs->dyn[24]=0.0;
  if (Prm.pipe_size<=0) 
  {
    result=1; 
    flag=1;
  }
  if ((Bs->dyn[12]<0.6)||(Bs->dyn[12]>0.75)||(Bs->dyn[16]>10)||(Bs->dyn[20]>5))
  {
    result=result+4; 
    flag=1;
  }
  if (Bs->dyn[0]<0) 
  {
    result=result+8; 
    flag=1;
  }
  if (Bs->dyn[4]<=0 || Bs->dyn[4]>5500)  
  {
    result=result+16;
    flag=1;
  }
  if (Bs->dyn[8]< -50 || Bs->dyn[8]>50)  
  {
    result=result+32;
    flag=1;
  }
  if (flag==1) 
  {
    Bs->dyn[24]=0.0; 
    goto M;
  } /*переход по ошибке*/

  azt=Bs->dyn[16]/100;
  ugl=Bs->dyn[20]/100;
  tabs=Bs->dyn[8]+273.15;
  pmega=Bs->dyn[4]/1E3;
  work_flow=Bs->dyn[0]; // m3/h

  std= CalcSFactor(Bs->dyn[12],azt,ugl);
  Pp= CalcPpk(Bs->dyn[12],azt,ugl);
  Tp= CalcTpk(Bs->dyn[12],azt,ugl);
  if (Prm.method==0)
  { 
    if (CalcNX19Koeff(Pp,Tp,pmega,tabs,std,&Z)!=0) result=result+4;
  }/*проверка*/
  else
  {
    if (CalcGERG91Koeff(azt,ugl,pmega,tabs,std,Bs->dyn[12],&Z)!=0)
    result=result+4;
  }
  Bs->dyn[32]=Z;
  pipe_m=Prm.pipe_size/1E3;
  Bs->dyn[39]= CalcViscosity(tabs,Bs->dyn[12],azt,ugl);
  if (Bs->dyn[4] > 500)
    Bs->dyn[39]= CalcViscExt(Bs->dyn[39],Pp,Tp,pmega,tabs);
  Bs->dyn[42]= CalcDensWork(Bs->dyn[12],Bs->dyn[4],tabs,Bs->dyn[32]);
  Bs->dyn[24]= CalcFlowRate(work_flow,Bs->dyn[4],tabs,Bs->dyn[32]);
  Bs->dyn[40]= CalcVelosity(Bs->dyn[12],Bs->dyn[24],Bs->dyn[42],pipe_m);
  Bs->dyn[38]= CalcReinAnnubar(Prm.sens_size,Bs->dyn[40],Bs->dyn[42],Bs->dyn[39]);
  M:return result;
}

/******** формирование архивной записи по типу архива и точкам ***/
unsigned char FormateArchive (struct station dv,struct dynparams bs[],
			  unsigned char typ_arc,unsigned char buf[],
			  struct expandparams exp)
{
  unsigned char i,j,ind_pnt,cr[4];
  float value;ind_pnt=0;
  for (i=0;i<Max_arch_pnt;i++)
    if (dv.arch[i][0] !=0 && dv.arch[i][1]<4 && dv.arch[i][3]<7)
  {
    value=0.0;
    switch (dv.arch[i][0])
    { 
      case 1:
        if (dv.arch[i][2]<Max_dyn_all)
	      switch (dv.arch[i][3])
	      {
	        case 1:value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]];break;
	        case 2:
	          switch (typ_arc)
	          {
		          case 0:value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]+2];break;
		          case 1:value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]+1];break;
		          case 2:value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]+7];break;
	          } 
          break;
	        case 3:
		        value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]+typ_arc+1];
          break;
	        case 4: case 6:
		        value=bs[dv.arch[i][1]].dyn[dv.arch[i][2]+dv.arch[i][3]];
          break;
	      } 
      break;
      case 2:
        switch (dv.arch[i][3])
	      {
	        case 1:value=exp.dyn[dv.arch[i][2]];break;
	        case 2:
	          switch (typ_arc)
	          {
		          case 0:value=exp.dyn[dv.arch[i][2]+2];break;
		          case 1:value=exp.dyn[dv.arch[i][2]+1];break;
		          case 2:value=exp.dyn[dv.arch[i][2]+7];break;
	          } 
          break;
	        case 3:
		        value=exp.dyn[dv.arch[i][2]+typ_arc+1];
          break;
	        case 4: case 6:
		        value=exp.dyn[dv.arch[i][2]+dv.arch[i][3]];
          break;
	      } 
      break;
    } 
    ConvToBynare(value,cr);
    for (j=0;j<4;j++) buf[7+ind_pnt*4+j]=cr[j];ind_pnt++;
  } 
  return ind_pnt;
}

/******************************************************************/