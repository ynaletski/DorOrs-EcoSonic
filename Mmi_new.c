
#include<7188xa.h>
#include<math.h>

//01.09.2020 YN -----\\//-----
//int zapret = 0;
int step =0;
unsigned char page_clear=2;
unsigned char page_temporary;
unsigned char page_str_pass=0;
unsigned char OK=9;
unsigned char count_menu=0;
//------------- -----//\\-----

#define Max_pnt           4   /*число точек учёта*/
#define Key_0        0x30
#define Key_1        0x31
#define Key_2        0x32
#define Key_3        0x33
#define Key_4        0x34
#define Key_5        0x35
#define Key_6        0x36
#define Key_7        0x37
#define Key_8        0x38
#define Key_9        0x39
#define Key_A        0x41
#define Key_B        0x42
#define Key_C        0x43
#define Key_D        0x44
#define Key_E        0x45
#define Key_F        0x46
#define Key_K        0x4b
#define Key_M        0x4d
#define Key_N        0x4e
#define Key_O        0x4f
#define Key_P        0x50
#define Key_T        0x54
#define Key_S        0x53
//01.09.2020 YN -----\\//-----
#define Key_G        0x47
//------------- -----//\\-----
#define Key_blank    0x20
#define Key_dot      0x2e
#define Key_comma    0x2c
#define Key_termin    0xd	//cr
#define Key_modbus    0xa
#define Key_mask      0xf
#define Key_minus    0x2d
#define Key_dies     0x23 //#
#define Key_cursor   0x5f
#define Key_usa      0x24 //$
#define Key_adr      0x40 //@
#define Len_name       16
#define Max_mmi_main   11
#define Max_mmi_point   7
#define Max_mmi_select  6
#define Max_mmi_exp     7
#define Len_select      4
#define Max_conf_all  101
#define Max_units_all  33
#define Max_sel_all   123
#define Max_number     40
#define Max_name_pnt    3
#define Max_dynam_all  71

struct cursor_mmi
{
  unsigned char enb; 
  unsigned char mode;
  unsigned char row,old;
  unsigned char size;
};

struct display_mmi
{
  unsigned char row,         /*строка от 0 до 5*/
		point,       /*номер выбранной точки*/
		flag,        /*признак обновления вывода списка имён*/
		num,         /*номер раширения от 0 до 6*/
		evt,         /*событие(всего три)*/
		write,       /*признак изменения конфигурации*/
		prm,         /*номер параметра в списке динам.параметров*/
		page,        /*страница дисплея*/
		old,         /*хранит страницу меню для возврата*/
	     suspend;        /*приостановка опроса клавиат для меню*/
  int pointer;
  unsigned char arch_num,size;
  unsigned char ret[5],ind_ret;
};

struct cursor_mmi             Cursor;
struct display_mmi            Display;

//01.09.2020 YN -----\\//-----
unsigned char *str_page[]=
{
	"                              ",
	"         Главное меню         ", //1+18
	"    Меню выбора точки учета   ", //2+18
	"  Меню выбора архивной точки  ", //3+18
	"     Переменные процесса      ", //4+19
	"        Ввод значения         ", //5+21
	" Изменение конфиг. параметра  ", //6+20
	"         Ввод пароля          ", //7+21
	"     Неправильный пароль      ", //8+22
	"   Просмотр архивной точки    ", //9+19
	"        Меню настроек         ", //10+18
	"       Выбор значения         ", //11+18
	"Изменение параметра невозможно", //12+22
	"Выполн.настр-ка нуля массомера", //13
	" Внимание! Перекройте расход  ", //14+15+16+23
	"через массомер, иначе возможен", //15
	" отказ его работы.            ", //16
	"     Меню выбора прибора      ", //17+18
	" F2 Пред, F3 След, Enter, ESC ", //18
	" F2 Пред, F3 След,        ESC ", //19
	" F1 Изменение      ESC Отмена ", //20
	" Enter Ввод        ESC Отмена ", //21
	"                   ESC Отмена ", //22	
	" F3 Продолжить       ESC Меню ", //23
	"    ООО Факом технолоджиз     ", //24+25
	"                     ESC Меню ", //25
	"   Вычислитель расхода ВРФ    ", //26+27+25
	"    Учет природного газа      "  //27
};

//------------- -----//\\-----
unsigned char *str_menu[]={
 "Общие настройки вычислителя",
 "Переменные процесса","Настройки процесса","Архив параметров суточный",
 "Архив параметров часовой","Настройки токовых модулей","Модули токового входа",
 "Многоканальные датчики","Счетчики импульсов","Инициализация модема","Коррекция дата-время"};

unsigned char *str_menu1[]={
 "Первая точка учета",
 "Вторая точка учета","Третья точка учета","Четвертая точка учета"};
unsigned char *str_menu2[]={"Первый прибор","Второй прибор","Третий прибор","Четвертый прибор"};

const unsigned char psw[10] = {205,229,228,238,241,242,243,239,229,237};
/*настройки порта*/
const unsigned char set_pr[3] = {0,2,1};
const unsigned char set_st[2] = {1,2};
const unsigned char set_dt[2] = {8,7};
const unsigned long set_bd[8] = {1200,2400,4800,9600,19200,38400,57600,115200};

const char *conf_all[]={
" ",										//20.09.2021 was: Выбор входа диффдавления
"Статус точки","Выбор сужающего устройства","Выбор входа раб.расхода",
"Выбор входа давления","Выбор входа температуры","Выбор входа вт. дифф",
"Статус алармов","Выбор теплоносителя","Межповерочный интервал",
"Единицы энергии","Диаметр трубопровода","Диаметр (ширина) СУ",
"Отсечка миним расхода","Верхний перекл диффман","Нижний перекл диффман",
"Начальный радиус кромки","Шероховатость трубопровода",
"КТР А трубопровода","КТР В трубопровода","КТР С трубопровода",
"КТР А сужающего устр","КТР В сужающего устр","КТР С сужающего устр",
"Нижний аларм расхода","Верхний аларм расхода","Нижний аларм диффдавл",
"Верхний аларм диффдавл","Гистерез алм диффдав","Нижний аларм давления",
"Верхний аларм давления","Гистерез алм давления","Нижний аларм температуры",
"Верхний аларм температ","Гистерез алм температ","Тег идентификатор",
"Адрес вычислителя","Контрактный час","Терминал оператора",
"Адрес датчика MVS #1","Адрес датчика MVS #2","Адрес датчика MVS #3",
"Адрес датчика MVS #4","Переход на зимнее/лет","Пароль оператора",
"Адрес модуля ICP #1","Адрес модуля ICP #2","Адрес модуля ICP #3",
"Адрес модуля ICP #4","Главный пароль","Телефонный номер",
"Настройка модема","Тип модуля ICP #1","Тип модуля ICP #2",
"Тип модуля ICP #3","Тип модуля ICP #4","Материал трубопровода",
"Материал суж устр","Конфигурация Modbus","Настройки порта COM",
"Архивная #","Способ термокоррекции","Конфигурация Hart",
"Коррекция мин расхода","Метод расчета","Единицы измерен AIN #",
"Нижняя граница AIN #","Верхняя граница AIN #","Параметр #",
"Плотность","Азот","Углекислый газ","Порт оператора","Data #",
"Обработка скрипта","Способ расчета кромки","Гистерез алм расхода",
"Единицы измерения AOT #","Нижняя граница AOT #",
"Верхняя граница AOT #"," "," "," "," "," "," "," "," "," "," ",
"Год","Месяц","День","Час","Минута"," "," "," "," ","Импульс/единица #",
"Сухость пара","Выбор входа расход",
};

 const char *dyn_all[]={
 " ",/*15.09.2021 "Диффдавление"*/"Рабочий расход","Давление абс","Температура",
 "Плотность","Расход мас","Расход обм","Энтальпия",
 "Масса за пр.час ","Объем за пр.час ","Масса месяц ","Энергия за пр.час ",
 "Азот","Угл газ","Объем месяц","Метан", "Этан","Пропан",
 "Энергия мес","н-Бутан","К сжимаемости",
 "К расхода","К расширения","К неостроты","К шероховатости",
 "К Рейнольдса","Число Рейнольдса","Вязкость",
 "Скорость потока","Давление насыщен","Показатель адиабаты",
 "Масса накоп","Объем накоп","и-Бутан","н-Пентан","и-Пентан",
 "Гексан","Энергия","Энергия нак","Гептан","Октан",
 "Нонан","Декан","Угар газ","Кислород","Водород","Сернист газ",
 "Гелий","Расход","Плотность раб","Канал "," "," "," "," ",
 " "," "," "," "," "," "," "," "," "," "," "," "," "," ",
 "Расход пр час ","К истечения"
 };

/*[0]-номер параметра
  [1]-адрес первого байта параметра
  [2]-признак пароля:1-пароль нужен
  [3]-тип данных:0-строка,1-байт,2-один.выбор,3-множ.выбор,
      4-веществ.,5-длин.целое*/
const char *name_mmi_exp[]={
 " "," час"," сутки"," п сут"," месяц", " п мес"," год"};

const char *units_all[]={
 " "," мм"," кПа"," грС"," м3/ч"," т/ч",
 " ГДж"," Гкал"," кг/м3"," Па*с"," м/с"," Е-6/гС",
 " кДж/кг"," т"," м3"," ГДж/ч"," Гкл/ч"," стм3/ч",
 " ст.м3"," ккл/кг"," моль %"," обм %"," год"," т стм3",
 " кг/ч"," литр/ч"," кг"," литры"," бл руб"," мПа*с",
 " %"," B"," г/см3"
};

const char *name_sel_all[]={
"Отключено","Включено","Диафрагма (Угловой)",
"Диафрагма (Фланцев)","Диафрагма (Трехрад)",
"Аннубар +10S","Аннубар +15/16S","Аннубар +25/26S",
"Аннубар +35/36S","Аннубар +45/46S","Аннубар 485_Size_1",
"Аннубар 485_Size_2","Аннубар 485_Size_3",
"Внешний расходомер","Константа","MVS #1","MVS #2","MVS #3",
"MVS #4","AIN #1","AIN #2","AIN #3","AIN #4","AIN #5",
"AIN #6","AIN #7","AIN #8","AIN #9","AIN #10","AIN #11",
"AIN #12","AIN #13","AIN #14","AIN #15","AIN #16",
"Модуль DIO первый","Модуль DIO второй","RTD #3","RTD #4",
"RTD #5","RTD #6","Вода","Пар","Стандарт (джоули)",
"Пользоват (калории)",/*15.09.2021 "Диффдавление"*/"Рабочий расход","Давление",
"Температура","Расход мгновенный","Брутто ",
" ",
"Подключен к СОМ1","Подключен к СОМ2","Авто",
"Зимнее","Модуль AIN первый","Модуль AIN второй",
"Модуль AOUT первый","Модуль AOUT второй","Материал",
"Коэффициент","Сталь 8","Сталь 10","Сталь 15",
"Ст 15М/20М","Сталь 16М","Сталь 20","Сталь 25",
"Сталь 30/35","Х6СМ/Х7СМ","12МХ","12Х18Н9/10Т",
"316 (S31600/CF8M)","14X17H2","15ХМА","15X1М10",
"15Х5М","17Х18Н9","20Х23Н13","36Х18Н25С2","301 (S30100)",
"12X1МФ","15Х1М1Ф","15Х12ЕНМФ","NX-19 mod","GERG-91 mod",
"ВНИЦ-СМВ","Фиксированная","Изменяемая","I-7060","I-7080",
"I-7040","Дозированное","До заполнения","От компьютера",
"Автономное","За период поверки","За время эксплуатац",
"Сталь 40/45","Сталь 20Л","Сталь 25Л","Сталь 35Л",
"Сталь 45Л","12Х18Н10Т/12Т","12Х18Н9ТЛ","25Х1МФ",
"18Х2Н4МА","12Х19Н9Т","10Х14Г14Н4Т","31Х19Н9МВБТ",
"37Х12Н8Г8МФБ"," "," "," ","Модуль PIN первый ","Диафрагма 1195",
"Модуль RTD","RTD #1","RTD #2","RTD #3","RTD #4",
"RTD #5","RTD #6"
};

const unsigned char name_mmi_select[Max_mmi_select][Len_select]={
 {0x22,0x31,0x22,0x20},{0x22,0x32,0x22,0x20},{0x22,0x33,0x22,0x20},
 {0x22,0x34,0x22,0x20},{0x22,0x35,0x22,0x20},{0x22,0x36,0x22,0x20},};
const unsigned char name_arch[Max_name_pnt][6]={{205,229,238,239,240,0x20},/*Неопр*/
 {193,224,231,238,226,Key_dies},/*Базов*/{196,238,239,238,235,237}/*Дополн*/};
unsigned char mmi_str[30],Size_str;/* буфер клавиатуры и дисплея третий порт */
unsigned char Vertical,Horizont,/*позиция символа 8*30 */
	      mmi_pass,/*номер прохода при отображении строки*/
	      enter_ind,/*в конфиг:число ввод символов*/
	      mmi_num_sel,/*число выборов для параметра*/
	      mmi_size,mmi_arc_page,flg_init;
unsigned char mmi_val[6],coord[8],count_smb,flg_modem;
int mmi_arc;/*указатель архива*/
unsigned mmi_seg,mmi_adr;/*сегмент и адрес*/
extern unsigned char size_max;
unsigned char mmi_sel[30];

//01.09.2020 YN -----\\//----
/********* установка строки страницы дисплея ****************************/
void page_screen(unsigned char hor, unsigned char ver, int str, unsigned char pass)
{
	int i;
	Horizont=hor;Vertical=ver;
	for (i=0;i<30;i++) mmi_str[i]=str_page[str][i];
	page_str_pass=pass;
}
//------------- -----//\\-----

/********* установка страницы дисплея ****************************/
void SetDisplayPage (unsigned char page)
{ 
	Display.evt=1; 
	//01.09.2020 YN -----\\//-----
	//Display.page=page;
	page_temporary=page;
	Display.page=40;
	//------------- -----//\\-----
}

/*************************/
void SaveOldPageMMI (void)
{
  Display.ret[Display.ind_ret]=Display.page; 
  Display.ind_ret++;
}

/*************************/
void GoToMenuMMI (unsigned char num_page)
{
  Cursor.enb=1; 
  Cursor.row=0; 
  Cursor.mode=1; 
  Display.flag=1;
  Display.row=0; 
  Cursor.size=0; 
  SaveOldPageMMI(); 
  Display.num=0;
  Display.suspend=1;
  SetDisplayPage(num_page);
}

/******* формирование запроса в MMI(интерфейс человек-машина)****/
unsigned char SendToMMI (unsigned char typ_port)
{
  unsigned char count,i,typ_pool;
  unsigned char buf_mmi[40],cr[2];
  buf_mmi[0]=Key_usa;
  buf_mmi[1]=Key_0;
  //01.09.2020 YN -----\\//-----
  buf_mmi[2]=Key_1; //was:Key_0;
  //------------- -----//\\-----
  switch (Display.evt)
  {
    case 0:buf_mmi[3]=Key_K;count=4;typ_pool=15;break;/*опрос клавиатуры*/
    case 1:
		buf_mmi[3]=Key_P;
		//01.09.2020 YN -----\\//-----     //was: вместо page_clear было Display.page
		buf_mmi[4]=hex_to_ascii[(page_clear >> 4) & Key_mask];
	    buf_mmi[5]=hex_to_ascii[page_clear & Key_mask];
			//------------ -----//\\-----
	   	count=6;
		typ_pool=16;
	break;/*установка страницы дисплея*/
	//01.09.2020 YN -----\\//-----
    case 2:
		buf_mmi[3]=Key_G;buf_mmi[4]=Key_S; 
		buf_mmi[5]=hex_to_ascii[(Horizont >> 4) & Key_mask];
		buf_mmi[6]=hex_to_ascii[Horizont & Key_mask];
		//buf_mmi[7]=Key_0;buf_mmi[8]=hex_to_ascii[Vertical];
		buf_mmi[7]=hex_to_ascii[((Vertical*8) >> 4) & Key_mask];
		buf_mmi[8]=hex_to_ascii[(Vertical*8) & Key_mask];
	   	//if (strlen(mmi_str)>15) {Display.evt = 4;step = 2;}
	   	if (count_smb>18) 
	    {
		    Display.evt = 4;
		    step = 2;
			for (i=0;i< 18;i++) buf_mmi[9+i]=mmi_str[i];
	   		count=9+18;typ_pool=17;
		}
		else
		{
	   		for (i=0;i< count_smb;i++) buf_mmi[9+i]=mmi_str[i];
	   		count=9+count_smb;typ_pool=17;
		}
	break;
	case 3:
		buf_mmi[3]=Key_S;
		count=4;typ_pool=18;
	break;/*опрос страницы*/
	//01.09.2020 YN -----\\//-----
	case 4:
		buf_mmi[3]=Key_G;buf_mmi[4]=Key_S; 
		buf_mmi[5]=hex_to_ascii[((Horizont+18) >> 4) & Key_mask];
		buf_mmi[6]=hex_to_ascii[(Horizont+18) & Key_mask];
		//buf_mmi[7]=Key_0;buf_mmi[8]=hex_to_ascii[Vertical];
		buf_mmi[7]=hex_to_ascii[((Vertical*8) >> 4) & Key_mask];
		buf_mmi[8]=hex_to_ascii[(Vertical*8) & Key_mask];
	   	for (i=0;i< (count_smb-18);i++) buf_mmi[9+i]=mmi_str[i+18];
	   	count=9+(count_smb-18);
		typ_pool=17;
		step=0;
	   	if (page_str_pass == OK) 
	   	{
		   Display.page=page_temporary;
		   page_str_pass=0;
		   switch (page_temporary)
		   {
		   case 16: Vertical=2; break;
		   case 15: case 17: Horizont=10;Vertical=3; break;
		   }
		   memset(mmi_str, 0, sizeof(mmi_str));
		}
	break;/*передача строки*/
	//------------- -----//\\-----
	default:count=0;typ_pool=0;Display.evt=0;break;
  } 
  if (count > 0)
  {
    CalcCheckSum(buf_mmi,count,cr);
	buf_mmi[count]=hex_to_ascii[cr[0]];
    buf_mmi[count+1]=hex_to_ascii[cr[1]];
	buf_mmi[count+2]=Key_termin;
    count=count+3;
	for (i=count;i>0;i--) buf_mmi[i]=buf_mmi[i-1];
    buf_mmi[0]=Key_termin;
	count++;
    if (typ_port==1) ToComBufn_1(buf_mmi,count);
	else if (typ_port==2) ToComBufn_2(buf_mmi,count);
	else if (typ_port==3) ToComBufn_3(buf_mmi,count);
  } 
  return typ_pool;
}

/********* проверка нажатой клавиши  *******************************/
unsigned char KeyFound (unsigned char buf_mmi[],unsigned char code_key1,
			unsigned char code_key2,unsigned char count)
{ /*используется для поиска пары кодов нажатой клавиши*/
  	unsigned char i,j;
	j=0;
  	for (i=4;i < count-1;)
  	{
   		if ((buf_mmi[i]==code_key1)&&(buf_mmi[i+1]==code_key2)) 
		{
			j=1;
			break;
		}
    i=i+2;
  } 
  if (j==0) return 0;
  else return 1;
}

/*********** очистка буфера вывода на экран *********************/
void ClearBuffer (void)
{
  unsigned char i;
  for (i=0;i< 30;i++) mmi_str[i]=Key_blank;
}

/************ преобразоваие в символьную строку ******************/
void FloatToString (float val,unsigned char buf_str[],
		   unsigned char offset)
{ /*используется при выводе конфиг.или архивного значений*/
  unsigned char i,buf[50],flag,j; 
  double musor,value;
  flag=j=0; if (val < 0) flag=1; 
  value=fabs(val);
  if (value < 100000)
  {
    musor=value;
    M: if (musor < 10) goto M1;
    musor=musor/10; j++; goto M;
    M1:
    value=value*pow(10,5-j); 
	musor=modf(value, &value);
    if (musor >= 0.5) value=value+1; 
	value=value/pow(10,5-j);
  } 
  else modf(value, &value); 
  if (flag == 1) value=-value;
  gcvt(value,10,buf); 
  j=0;
  for (i=0;i<10;i++) 
  if (buf[i] !=0) buf_str[i+offset]=buf[i]; 
  else break;
}

/*********** преобразование байта в два символа ***********/
void ByteToString (unsigned char val,unsigned char index,unsigned char typ)
{ /*используется при выводе архивных даты и времени*/
  unsigned char a,d,e,c,b;
  c=10;
  b=val;
  a=0;
  while (c>=1)
  {
    d=b/c; 
    if (typ==1 && a==0 && d==0) 
    {
		mmi_str[index]=0x20;
		index++;
		goto M;
	}
    if (d < 10)
    { 
		mmi_str[index]=hex_to_ascii[d & Key_mask];
		index++;
	} 
	else
    {
      e=d/10; 
	  mmi_str[index]=hex_to_ascii[e & Key_mask];
	  index++;
      e=d-e*10; 
	  mmi_str[index]=hex_to_ascii[e & Key_mask];
	  index++;
    } 
M:  b=b-d*c;
	c=c/10;a++;
  }
}

/************** преобразование целого в сторку ******************/
unsigned char IntegerToString (unsigned long val)
{
  unsigned long d;
  unsigned char buf[10],i,j;
  i=0;
  while (val > 0)
  {
    d=val/10;buf[i]=hex_to_ascii[(val-d*10) & Key_mask];
    i++;
	val=d;
  } 
  for (j=0;j<i;j++) mmi_str[j]=buf[i-j-1];
  return i;
}

/*********** отображение введённого знака *******************/
void EnterKey (unsigned char code_symbol,unsigned char *index)
{ /*используется при вводе пароля или конфиг.значения*/
   if (*index < 8)
   {
     	mmi_str[*index]=code_symbol; 
		count_smb=8;
     	mmi_str[*index+1]=Key_cursor;
		*index=*index+1;
		Display.evt=2;
   }
}

/*********** удаление введённого знака ***********************/
void DeleteKey (unsigned char *index)
{ /*используется при вводе пароля или конфиг.значения*/
   if (index > 0)
   {
     mmi_str[*index]=Key_blank;
	 *index=*index-1;
     mmi_str[*index]=Key_cursor;
	 count_smb=8;
	 Display.evt=2;
   }
}

/** ввод цифрового значения для:пароля,измен.конфиг,архивов **/
unsigned char EnterFigure (unsigned char buf_mmi[],unsigned char count)
{ /*коды клавиш от "0" до "9",перевод в символ и цифру*/
  	const unsigned char dec_key[10]={Key_3,Key_C,Key_D,Key_F,Key_E,
		Key_8,Key_9,Key_B,Key_A,Key_1};
  	unsigned char i,flag;
	/*num_key=16;*/
	flag=0;
  	if (enter_ind < 9) for (i=0;i<10;i++)
  	{
    	if (KeyFound (buf_mmi,Key_0,dec_key[i],count)==1)
    	{ 
			EnterKey(Key_0+i,&enter_ind);
			flag=1;
			break; 
		}
	} 
	return flag;
}

/********* возврат введённой цифры  ******************************/
unsigned char ConvertToDec (unsigned char symbol)
{/*код"10"минус,код"11"запятая,перевод десятичного символа в цифру*/
  unsigned char dec;
  if ((symbol>47)&&(symbol<58)) dec=symbol-48;
  if (symbol==Key_minus) dec=10;
  if (symbol==Key_dot) dec=11;
  return dec;
}

/******* показывает позицию для ввода символа ******************/
void ViewCursor (unsigned char index)
{
   	if (index==0)
   	{ 
	   mmi_str[0]=Key_cursor;
	   count_smb=8;
	   Display.evt=2;
	}
}

/********* получение новых настроек архива при чтении *************/
void GetArcReadPoint(int *pointer,unsigned *segment,unsigned char *page)
{
  switch (*page)
  {
    case 0:*page=2;*segment=0xc000;
	   *pointer=ReadNVRAM(17)*256+ReadNVRAM(18);break;
    case 1:*page=0;*segment=0xd000;
	   *pointer=ReadNVRAM(13)*256+ReadNVRAM(14);break;
    case 2:*page=1;*segment=0xe000;
	   *pointer=ReadNVRAM(15)*256+ReadNVRAM(16);break;
  }
}

/**********************************/
void MoveCursorMMI (unsigned char buf_mmi[],unsigned char broad,
                                             unsigned char count)
{
  M1:if (Cursor.enb==1)
    {
      if (Cursor.mode==1) 
      {
		Horizont=0; Vertical=Cursor.row+1;
		//01.09.2020 YN -----\\//-----
		Vertical+=1;
		//------------- -----//\\-----
	 	count_smb=1; ClearBuffer();
        Cursor.enb=0; mmi_str[0]=0x3e; Display.evt=2; goto M2;
      } else
      {
		Vertical=Cursor.old+1; 
		//01.09.2020 YN -----\\//-----
		Vertical+=1;
		//------------- -----//\\-----
		count_smb=1; ClearBuffer(); Display.evt=2;
        Cursor.mode=1; Horizont=0; goto M2;
      }
    } else if (KeyFound(buf_mmi,Key_0,Key_3,count)==1 && Cursor.row<(broad-1)) //KEY 0
    {
      Cursor.old=Cursor.row; Cursor.row=Cursor.row+1; Cursor.enb=1;
      Cursor.mode=0; goto M1;
    } else if (KeyFound(buf_mmi,Key_0,Key_F,count)==1 && Cursor.row>0) //KEY 3
    {
      Cursor.old=Cursor.row; Cursor.row=Cursor.row-1; Cursor.enb=1;
      Cursor.mode=0; goto M1;
    }
  M2:
}
/******************************/
void ReturnToMenuMMI (void)
{
  unsigned char num_page;
  Cursor.enb=1; 
  Cursor.row=0; 
  Cursor.mode=1; 
  Display.flag=1;
  Display.row=0; 
  Cursor.size=0; 
  Display.suspend=1; 
  if (Display.ind_ret>0) 
  {
    num_page=Display.ret[Display.ind_ret-1]; 
	Display.ind_ret--;
  } 
  else num_page=10; 
  SetDisplayPage(num_page);
}

/*********** изменение конфигурации и запись события ***************/
unsigned char ConfigParamChange (unsigned char value[],unsigned char size,
				 unsigned char addr)
{
  unsigned char buf_evt[16],j,flag;
  flag=0;
  FormateEvent(buf_evt);
  for (j=0;j< size;j++)
  {
    if (value[j] != mmi_val[j])
    {
      EnableEEP();
	  WriteEEP(Display.point,addr+j,value[j]);
	  ProtectEEP();
      flag=1;
    } 
	buf_evt[6+j]=mmi_val[j];
	buf_evt[10+j]=value[j];
  }
  if (flag == 1) /*запись события от терминала*/
  {
    buf_evt[15]=Display.point+1+128;
	buf_evt[14]=Display.prm;
	WriteEvent(buf_evt,0);
    return Display.point+1;
  } 
  else return 0;
}

/******************************/
unsigned char  MoveListMMI (unsigned char buf_mmi[],unsigned char count,
                  unsigned char size_max)
{
  unsigned char flag;
//01.09.2020 YN -----\\//-----        										was:6; now: >=12 -12 -12 +12;
  if (KeyFound(buf_mmi,Key_0,Key_5,count)==1 && Display.num>=12) //F2
  {
    Display.num=Display.num-12;
	Horizont=0;
	Display.row=0;
	Display.flag=1;
	flag=1;
    Cursor.size=0;
	Display.suspend=1;
  } 
  else if (KeyFound(buf_mmi,Key_0,Key_7,count)==1 && Display.num<(size_max-12)) //F3
  {
    Display.num=Display.num+12;
	Horizont=0;Display.row=0;
	Display.flag=1;
	flag=1;
	//------------- -----//\\-----
    Cursor.size=0;
	Display.suspend=1;
  } 
  else flag=0; 
  return flag;
}

/******************************/
void WriteMenuToMMI (unsigned char menu[], unsigned char size_menu)
{
  unsigned char i,j,flag;
  ClearBuffer(); Vertical=Display.row+1;
  //01.09.2020 YN -----\\//-----
  Vertical += 1;
  //------------- -----//\\-----
  if ((Display.num+Display.row)<size_menu)
  {
    j=strlen(menu); for (i=0;i<j;i++) mmi_str[i]=menu[i];
    Cursor.size++;
  } 
  count_smb=28;
  Horizont=2;
  Display.evt=2;
  Display.row++;
//01.09.2020 YN -----\\//-----
  if (size_menu > 11) count_menu=11;
  else count_menu=size_menu;
  if (Display.row > count_menu) //was:5 колличество строк в меню
//------------- -----//\\-----
  {
    Display.row=0; 
	Display.flag=0; 
	Cursor.mode=0; 
	Cursor.old=Cursor.row;
    Cursor.row=0; 
	Cursor.enb=1;
	Display.suspend=0;
  }
}

/******* обработка ответа от MMI(терминал оператора) *****/
void ReadFromMMI (unsigned char buf_mmi[],unsigned char count,
		  double dyn_prm)
{
  /*[]-[]-[]-*/
  unsigned char i,j,k,cr[4],buf_evt[16],flg;float value;int pnt_evt;
  switch (Display.page)
  {
    case 0: case 1: case 2: case 3: case 4: case 5:
    case 6: case 7: case 8: case 9:/*заголовок*/
      if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) /*"D"*/
      GoToMenuMMI(10); else
      { /* визуализация контрольной суммы */
		ClearBuffer(); 
		Display.suspend=0; 
		k=0;
        for (i=0;i<3;i++) /*выводит дату*/
        { 
          ByteToString(ReadNVRAM(2-i),k,0);if (i != 2) mmi_str[k+2]=Key_dot;
		  k=k+3;
		} 
		k=9;
		count_smb=18;
		for (i=0;i<3;i++) /*выводит время*/
		{ 
          	ByteToString(ReadNVRAM(3+i),k,0);if (i != 2) mmi_str[k+2]=0x3a;k=k+3;
		} 
		Horizont=5;
		Vertical=4;
		Display.evt=2;
      } 
	break;
    case 10:/*основное меню*/
       	if (Display.flag==1)
       		WriteMenuToMMI(str_menu[Display.num+Display.row],11); 
	   	else
       		MoveCursorMMI(buf_mmi,Cursor.size,count);
       	MoveListMMI (buf_mmi,count,10);
       	if (KeyFound(buf_mmi,Key_0,Key_6,count)==1) 
       	{ 
		   Display.flag=0;
		   ReturnToMenuMMI();
		} 
		else if (KeyFound(buf_mmi,Key_0,Key_2,count)==1) 
       	switch (Cursor.row+Display.num)
       	{ 
	 		case 0: Display.point=7; Display.write=0; GoToMenuMMI(20);break;
	 		case 1: GoToMenuMMI(11); break;
	 		case 2: GoToMenuMMI(12); break;
	 		case 3: Display.point=1; GoToMenuMMI(13); break;
	 		case 4: Display.point=0; GoToMenuMMI(13); break;
	 		case 5: Display.point=6; Display.write=0; GoToMenuMMI(20);break;
	 		case 6: Display.prm=5; enter_ind=2;GoToMenuMMI(23); break;
	 		case 7: Display.prm=7; enter_ind=4;GoToMenuMMI(23); break;
	 		case 8: 
			 	Display.point=11; 
				Display.row=Horizont=mmi_pass=0;
		 		Display.flag=1;
				 Display.num=0;
				 SetDisplayPage(14);
			break;
	 		case 9: 
			 	flg_modem=2; 
				ClearBuffer();
				Horizont=10;
				Vertical=3;
		 		SetDisplayPage(17);
			break;
	 		case 10:Display.point=60; Display.write=0;GoToMenuMMI(20);break;
       	}
	break;
    case 11:/*меню выбора точки для просмотра*/
      	if (Display.flag==1) WriteMenuToMMI(str_menu1[Display.num+Display.row],Max_pnt); 
		else MoveCursorMMI(buf_mmi,Cursor.size,count);
      	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) ReturnToMenuMMI(); 
		else if (KeyFound (buf_mmi,Key_0,Key_2,count)==1)
      	{
			Display.point=Cursor.row; 
			Horizont=0; 
			Display.row=0; 
			Display.num=0; 
			Display.flag=1;
			mmi_pass=0; 
			Display.suspend=1;
			SaveOldPageMMI(); 
			SetDisplayPage(14);
      	} 
	break;
    case 12:/*меню выбора точки для конфигурации*/
      	if (Display.flag==1) WriteMenuToMMI(str_menu1[Display.num+Display.row],Max_pnt); 
		else MoveCursorMMI(buf_mmi,Cursor.size,count);
      	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) ReturnToMenuMMI(); 
		else if (KeyFound (buf_mmi,Key_0,Key_2,count)==1)
      	{ 
			Display.point=Cursor.row; 
			Display.write=0; 
			GoToMenuMMI(20);
		} 
	break;
    case 14:/*просмотр до 6 параметров*/
		if (KeyFound (buf_mmi,Key_0,Key_6,count)==1)
		{
			Display.num=0;
			ReturnToMenuMMI();
		}
		else/*"D"переход в меню выбора точки для просмотра*/
		if (MoveListMMI (buf_mmi,count,size_max)==1) mmi_pass=0;
		else
        {
	  		ClearBuffer();Vertical=Display.row+1;
	  		//01.09.2020 YN -----\\//-----
	  		Vertical+=1;
	  		//------------- -----//\\-----
	  		if (mmi_pass==0) /*смена списка*/
	  		{  /*вывод имени параметра за один проход*/
	    		if ((Display.num+Display.row)<size_max && coord[0]<Max_dynam_all)
	    		{
	      			j=strlen(dyn_all[coord[0]]);
	      			for (i=0;i<j;i++) mmi_str[i]=dyn_all[coord[0]][i];
	      			if (coord[2]==2 && coord[1]<Max_mmi_exp && coord[1]>0)
	      			{
	        			k=strlen(name_mmi_exp[coord[1]]);
						for (i=j;i<j+k;i++)
							mmi_str[i]=name_mmi_exp[coord[1]][i-j];
						mmi_size=j+k;
	     			} 
					else mmi_size=j; 
					j=strlen(units_all[coord[3]]);
	      			for (i=mmi_size;i<mmi_size+j;i++)
	      			if (coord[3] < Max_units_all)
	      				mmi_str[i]=units_all[coord[3]][i-mmi_size];
              		if (coord[4]==1) ByteToString(coord[5],mmi_size,1);
	    		} 
				count_smb=22;
				Horizont=0;
				mmi_pass=1;
				Display.evt=2;
	  		} 
			else
	  		{ /*вывод значения параметра*/
	      		if((Display.num+Display.row)<size_max)
					FloatToString(dyn_prm,mmi_str,0);
	      		count_smb=8;
				Horizont=22;
				Display.row++;
		  		//01.09.2020 YN -----\\//-----
	      		if (Display.row > 11) //was: >5 колличество строк в меню
		  			//------------- -----//\\----- 
				{
				  	Display.row=Display.flag=0;
				 	 Display.suspend=0;
				}	
              	if (Display.flag==1) mmi_pass=0;
				Display.evt=2;
	  		}
		} 
	break;
    case 15:/*ввод значения*/
		if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) /*отмена измен конфиг*/
		{
	  		mmi_pass=Display.write=enter_ind=0;
			Horizont=0;
			Display.flag=1;
	  		Vertical=2;
			SetDisplayPage(16);
		} 
		else if (KeyFound (buf_mmi,Key_1,Key_E,count)==1) DeleteKey(&enter_ind);
		else if (EnterFigure(buf_mmi,count) != 1)
		{
	  		if (KeyFound (buf_mmi,Key_1,Key_B,count)==1) /*запятая*/
	      	EnterKey(Key_dot,&enter_ind);
			else
	  		if (KeyFound (buf_mmi,Key_0,Key_2,count)==1 && enter_ind>0)
	  		{ /*ввод нового конфигурационного значения*/
	    		value=atof(mmi_str);
	    		if (coord[3] == 1 || coord[3] == 2)
	    		{ /*для байтовых значений разделитель отсутствует*/
	      			cr[0]=value;
					flg_init=ConfigParamChange(cr,1,coord[2]);
	    		}
	    		if (coord[3] == 4)
	    		{ /*для действительных значений*/
	      			ConvToBynare(value,cr);
	      			flg_init=ConfigParamChange(cr,4,coord[2]);
	    		}
	    		if (coord[3] == 60 && coord[2]<5)
	    		{
	      			cr[0]=value; 
					WriteNVRAM(coord[2],cr[0]);
	     	 		if (coord[2] == 4) WriteNVRAM(5,0);
	      			if (coord[2] >= 3)
					SetTime(ReadNVRAM(3),ReadNVRAM(4),ReadNVRAM(5));
	      			if (coord[2] < 3)
					SetDate(ReadNVRAM(0)+2000,ReadNVRAM(1),ReadNVRAM(2));
	    		} 
				mmi_pass=Display.write=enter_ind=0;
				Horizont=0;
				Display.flag=1;
	      		Vertical=2;
				SetDisplayPage(16);/*визуализация изменения*/
	  		} 
		  	else ViewCursor(enter_ind);
		} 
	break;
    case 16:/*изменение конфигурации*/
		if (Display.flag==1)
		{ 
			ClearBuffer();
	  		if (mmi_pass==0)
	  		{
	    	j=strlen(conf_all[coord[0]]);
	    	for (i=0;i<j;i++)
	    	mmi_str[i]=conf_all[coord[0]][i];
			k=strlen(units_all[coord[5]]);
	    	if (coord[5]>0)
	    	for (i=j;i<j+k;i++)
	    		mmi_str[i]=units_all[coord[5]][i-j];
			else if (coord[4]<=40) ByteToString(coord[4]+1,j,1);
	    	count_smb=28;
			mmi_pass=1;
	  	} 
	  	else
	  	{
	    	if (coord[3]==4)
	    	{
	      		ConvToFloatVerify(&value,ReadEEP(Display.point,coord[2]),
					ReadEEP(Display.point,coord[2]+1),ReadEEP(Display.point,coord[2]+2),
						ReadEEP(Display.point,coord[2]+3));
	      		FloatToString(value,mmi_str,0);
	    	} 
			else if (coord[3]==1 || coord[3]==3)
	    	{
	      		mmi_val[0]=ReadEEP(Display.point,coord[2]);
	      		ByteToString(mmi_val[0],0,1);	      
	    	} 
			else if (coord[3]==2)
	    	{
	     		mmi_val[0]=ReadEEP(Display.point,coord[2]);
	      		j=strlen(name_sel_all[mmi_sel[mmi_val[0]]]);
	      		if (mmi_val[0]<30 && mmi_sel[mmi_val[0]]<Max_sel_all)
	      			for (i=0;i<j;i++)
	      				mmi_str[i]=name_sel_all[mmi_sel[mmi_val[0]]][i];
	    	} 
			else if (coord[3]==6)
	    	{
	      		mmi_val[0]=ReadEEP(Display.point,coord[2]);
	      		i=IntegerToString(set_bd[mmi_val[0] & 15]);
	      		mmi_str[i]=mmi_str[i+2]=Key_comma;
	      		mmi_str[i+1]=hex_to_ascii[set_dt[(mmi_val[0] &16)>>4] & Key_mask];
	    		switch ((mmi_val[0] &192)>>6)
	      		{
					case 0:mmi_str[i+3]=Key_N;break;
					case 1:mmi_str[i+3]=Key_O;break;
					case 2:mmi_str[i+3]=Key_E;break;
	      		}
	    	} 
			else if (coord[3]==0 || coord[3]==10)
	    	{
	      		for (i=0;i<8;i++) mmi_str[i]=ReadEEP(Display.point,coord[2]+i);
	    	} 
			else if (coord[3]==5 || coord[3]==9)
	    	{
	      		for (i=0;i<10;i++) mmi_str[i]=psw[i];
	    	}	 
			else if (coord[3] == 8)
	    	{
	      		mmi_val[0]=ReadEEP(Display.point,coord[2]);
	      		if (mmi_val[0] < Max_name_pnt) for (i=0;i<6;i++)
	      			mmi_str[i]=name_arch[mmi_val[0]][i];
	      		if (mmi_val[0] != 0)
	      		{
					if (mmi_val[0] == 1)
					{
		  				mmi_val[1]=ReadEEP(Display.point,coord[2]+1)+1;
		  				mmi_str[6]=hex_to_ascii[mmi_val[1] & Key_mask];
					}
					mmi_str[7]=Key_comma;
					j=strlen(dyn_all[coord[7]]);
					if (coord[7]> 0 && coord[7]<Max_dynam_all)
					for (i=0;i<j;i++)
					mmi_str[8+i]=dyn_all[coord[7]][i];
					mmi_val[3]=ReadEEP(Display.point,coord[2]+3);
					if (mmi_val[3]>3 && mmi_val[3]<Max_mmi_exp)
						for (i=0;i<6;i++)
							mmi_str[8+j+i]=name_mmi_exp[mmi_val[3]][i];
	     		} 
	    	} 
			if (coord[3] == 12)
	    	{
	     		mmi_val[0]=ReadEEP(Display.point,coord[2]);
	      		k=strlen(units_all[mmi_val[0]]);
	      		if (mmi_val[0]<Max_units_all) for (i=0;i<k;i++)
	      			mmi_str[i]=units_all[mmi_val[0]][i];
	    	} 
			if (coord[3] == 60)
	    	{ 
				mmi_val[0]=ReadNVRAM(coord[2]);
				ByteToString(mmi_val[0],0,1);
			}
	    	Horizont=2;
			Vertical=4;
			count_smb=29;
			Display.flag=0;
	  	}   
		Display.evt=2;
	} 
	else if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) /*"ESC"переход в меню */
	{ 
		Display.num=Display.old; 
		ReturnToMenuMMI();
	} 
	else if (KeyFound (buf_mmi,Key_0,Key_4,count)==1)
	{
	  if (coord[6] !=2) /*"F1"ввод пароля для изменения*/
	  {
	    ClearBuffer();
		Horizont=10;
		Vertical=3;
	    if (coord[6]==1) SetDisplayPage(17);
		else
	    {
			Display.write=1;
			enter_ind=0;
			SetDisplayPage(15);
		}
	  } 
	  else  SetDisplayPage(22);
	} 
	break;
    case 17:/*ввод пароля*/
       if (KeyFound(buf_mmi,Key_0,Key_6,count)==1) /*отказ от изменения*/
       {
	 		if (flg_modem==0)
	 		{
	   			mmi_pass=0;
				Horizont=0;
				Display.flag=1;
				Display.write=0;
				enter_ind=0;
	   			Vertical=2; 
				flg_modem=0; 
				SetDisplayPage(16);
	 		} 
			else
	 		{
	   			flg_modem=0; 
				Cursor.enb=1; 
				Cursor.row=0;
	   			Cursor.mode=1; 
				Display.flag=1;
				Display.row=0; 
				SetDisplayPage(10);
	 		}
       	} 
		else
       	{
	 		if (KeyFound(buf_mmi,Key_1,Key_E,count)==1) DeleteKey(&enter_ind);
			else/*удаление последнего*/
	 		if (EnterFigure(buf_mmi,count)!=1);/*ввод значений от "0"до"9"*/
	 		{
	   			if (KeyFound(buf_mmi,Key_0,Key_2,count)==1 && enter_ind>0)
	   			{ 	/*ввод пароля на изменение конфигурации*/
	     			/*value=ConvertToNumber(enter_ind); /*вычисление эталонного пароля*/
	     			value=atof(mmi_str);
	     			if (value != dyn_prm) SetDisplayPage(18);
					else
	     			{ /*пароль введён правильно*/
	       				ClearBuffer();
						if (coord[3]==2)
	       				{
		 					Horizont=Display.row=Display.num=0; 
							Display.flag=1;
		 					SetDisplayPage(21);
	       				} 
						else if (flg_modem==2)
	       				{
		 					flg_modem=1; 
							Cursor.enb=1; 
							Cursor.row=0; 
							enter_ind=0;
		 					Cursor.mode=1; 
							Display.flag=1;
							Display.row=0; 
							SetDisplayPage(10);
	       				} 
						else
	       				{
		 					SetDisplayPage(15);
							Display.write=1;
							enter_ind=0;
							Horizont=10;
	       				}
	     			}
	 			} 
				else ViewCursor(enter_ind);
       		}
      	} 
	break;
    case 18: case 22:/*неверный пароль или запрет*/
      	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) /*"ESC"*/
     	{
			mmi_pass=0;
			Horizont=0;
			Display.flag=1;
			Display.write=0;
			enter_ind=0;
			Vertical=2;
			SetDisplayPage(16);
      	} 
	break;
    case 20:/*подменю*/
      	if (MoveListMMI (buf_mmi,count,Display.size)==0)
      	{ 
        	if (Display.flag==1) 
        	{
          		ClearBuffer();Vertical=Display.row+1;
				//01.09.2020 YN -----\\//-----
	  			Vertical+=1;
	  			//------------- -----//\\-----
        		if ((Display.num+Display.row)<Display.size) 
	  			{
	    			j=strlen(conf_all[coord[0]]);
	    			for (i=0;i<j;i++) mmi_str[i]=conf_all[coord[0]][i];
	    			if (coord[1]<=40) ByteToString(coord[1]+1,j,1);
        			Cursor.size++;
         		} 
				count_smb=27;
				Horizont=2;
				Display.evt=2;
				Display.row++;
				//01.09.2020 YN -----\\//-----
      			if (Display.row > 11)  //was: >5 колличество точек меню
				//------------- -----//\\-----
          		{
            		Display.row=0;
					Display.flag=0;
					Cursor.mode=0;
					Cursor.old=Cursor.row;
	    			Cursor.row=0;
					Cursor.enb=1;//Display.suspend=0;
	  			}
			} 
			else MoveCursorMMI(buf_mmi,Cursor.size,count);
        	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1)
        	{ 
				Display.num=0; 
				ReturnToMenuMMI();
			} 
			else if (KeyFound (buf_mmi,Key_0,Key_2,count)==1)
			{
	  			Display.prm=Display.num+Cursor.row;
				mmi_pass=0;
				Display.flag=1;
	  			Display.write=0;
				Horizont=0;
				enter_ind=0;
				Vertical=2; 
				SaveOldPageMMI();
	  			Display.old=Display.num; 
				SetDisplayPage(16);
        	}
      	} 
	break;
    case 21:/*просмотр до 6 пунктов выбора значения*/
      	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1)
      	{
			mmi_pass=Display.write=enter_ind=Horizont=0;
			Display.num=Display.old;
			Display.flag=1; 
			Vertical=2; 
			SetDisplayPage(16);
      	} 
		else/*"ESC"отмена изменения параметра*/
      	{
			MoveListMMI (buf_mmi,count,mmi_num_sel);
			ClearBuffer(); 
			Vertical=Display.row+1;
			//01.09.2020 YN -----\\//-----
			Vertical+=1;
			//------------- -----//\\-----
			if (Display.flag==1) /*смена списка*/
			{ /*вывод пункта меню выбор значения за один проход*/
	  			if ((Display.num+Display.row)<mmi_num_sel)
	  			{
	    			j=strlen(name_sel_all[mmi_sel[Display.num+Display.row]]);
	    			for (i=0;i<j;i++)
	    			mmi_str[i+3]=name_sel_all[mmi_sel[Display.num+Display.row]][i];
	    			Cursor.size++;
	  			} 
				count_smb=30; 
				Horizont=0; 
				Display.evt=2; 
				Display.row++;
				//01.09.2020 YN -----\\//-----
      			if (Display.row > 11)  //was: >5 колличество точек меню
				//------------- -----//\\-----
	  			{
	    			Display.row=Display.flag=0;
					Cursor.mode=0;
					Cursor.old=Cursor.row;
	    			Cursor.row=0; 
					Cursor.enb=1; //Display.suspend=0;
	  			}
			} 
			else MoveCursorMMI(buf_mmi,Cursor.size,count);
			if (KeyFound (buf_mmi,Key_0,Key_2,count)==1)
			{
	  			cr[0]=Display.num+Cursor.row;
				mmi_pass=0;
				Horizont=0;
				Display.flag=1;
	  			flg_init=ConfigParamChange(cr,1,coord[2]); 
				enter_ind=0;
	  			Vertical=2; 
				SetDisplayPage(16);
			}
      	} 
	break;
    case 13:/*просмотр до 6 пунктов меню арх точек*/
      	if (MoveListMMI (buf_mmi,count,size_max)==0)
      	{
			if (Display.flag==1)
			{
	  			ClearBuffer();
				Vertical=Display.row+1;
		  		//01.09.2020 YN -----\\//-----
	  			Vertical+=1;
	  			//------------- -----//\\-----
          		if ((Display.num+Display.row)<size_max)
	  			{ 
            		if (coord[0]<Max_conf_all) for (i=0;i<5;i++)
	      				mmi_str[i+2]=conf_all[coord[0]][i];
	      			ByteToString(coord[1],8,0);
	      			if (coord[2]<3) for (i=0;i<6;i++)
	      				mmi_str[i+10]=name_arch[coord[2]][i];
	      			if (coord[2]==1) mmi_str[16]=hex_to_ascii[coord[3]+1];
	      			if (coord[2]>0)
	      			{
						j=strlen(dyn_all[coord[4]]);
						if (coord[4]<Max_dynam_all) for (i=0;i<j;i++)
							mmi_str[i+17]=dyn_all[coord[4]][i];
						mmi_val[Display.row]=coord[6];k=strlen(name_mmi_exp[coord[5]]);
						if (coord[5]<Max_mmi_exp) if (coord[5]==4 || coord[5]==6)
						for (i=j;i<k+j;i++) if ((i+17)<30)
						mmi_str[i+17]=name_mmi_exp[coord[5]][i-j];
	      			} 
					else mmi_val[Display.row]=128; 
					Cursor.size++;
	   			} 
				count_smb=30;
				Horizont=0;
				Display.evt=2;
				Display.row++;
	   			//01.09.2020 YN -----\\//-----
          		if (Display.row > 11)  //was: >5 колличество точек меню
				//------------- -----//\\----- 
	  			{
	    			Display.row=Display.flag=0;
					Cursor.mode=0;
					Cursor.old=Cursor.row;
	    			Cursor.row=0;
					Cursor.enb=1;
					Display.suspend=0;
	  			}
			} 
			else MoveCursorMMI(buf_mmi,Cursor.size,count);
			if (KeyFound (buf_mmi,Key_0,Key_6,count)==1)
			{ 
				Display.num=0; 
				ReturnToMenuMMI();
			} 
			else if (KeyFound (buf_mmi,Key_0,Key_2,count)==1 && mmi_val[Cursor.row]!=128)
			{
	  			mmi_arc_page=GetArcPoint(&mmi_arc,&mmi_seg); 
				Display.flag=1;
	  			mmi_size=0;
				Display.prm=Display.num+Cursor.row;
				SetDisplayPage(19);
			}
      	} 
	break;
    case 19:
      	if (KeyFound (buf_mmi,Key_0,Key_6,count)==1)
      	{
			Display.flag=1;
			Display.row=0;
			Display.suspend=1;
			SetDisplayPage(13);
		} 
		else
      	{ /*"C"листать архив вниз*/
			if (KeyFound (buf_mmi,Key_0,Key_7,count)==1 && mmi_size<2)
			{
				Display.row=0;
				Display.flag=1;
			} 
			else
			{
	  			if (mmi_arc<=0 && mmi_size<2) /*начало следующего прохода*/
	  			{
					mmi_size++;
					GetArcReadPoint(&mmi_arc,&mmi_seg,&mmi_arc_page);
				} 
				else
	  			M:if (Display.flag == 1)
	  			{
	    			ClearBuffer();
					Vertical=Display.row+1;
					//01.09.2020 YN -----\\//-----
	  				Vertical+=1;
	  				//------------- -----//\\-----
	    			if (mmi_arc > 0)
	    			{
	      				mmi_arc--;
						mmi_adr=mmi_arc*Size_str;
	      				if (FlashRead(mmi_seg,mmi_adr)==Display.point)
	      				{
							k=0;
							for (i=0;i<3;i++)
							{ /*прочитать и показать дату архива*/
		  						j=FlashRead(mmi_seg,mmi_adr+3-i);
								ByteToString(j,k,0);
		  						if (i!=2) mmi_str[k+2]=Key_dot;
								k=k+3;
							} 
							k=9;
							for (i=0;i<2;i++)
							{ /*прочитать и показать время архива*/
		  						j=FlashRead(mmi_seg,mmi_adr+4+i);
								ByteToString(j,k,0);
		  						if (i==0) mmi_str[k+2]=0x3a;
								k=k+3;
							} 
							k=15;
							j=7+(Display.prm)*4;
							ConvToFloatVerify(&value,FlashRead(mmi_seg,mmi_adr+j),
								FlashRead(mmi_seg,mmi_adr+j+1),FlashRead(mmi_seg,mmi_adr+j+2),
									FlashRead(mmi_seg,mmi_adr+j+3));
							FloatToString(value,mmi_str,k);
	      				} 
						else goto M;
	    			} 
					count_smb=24;
					Display.row++;
					Display.evt=2;
					//01.09.2020 YN -----\\//-----
	    			if (Display.row > 11) //was: >5 колличество точек меню 
					//------------- -----//\\-----
					Display.flag=0;Horizont=0;
	  			}
			}
      		} 
		break;
    	case 23: /*меню выбора датчика*/
     		if (Display.flag==1) WriteMenuToMMI(str_menu2[Display.num+Display.row],enter_ind); 
			else MoveCursorMMI(buf_mmi,Cursor.size,count);
      		if (KeyFound (buf_mmi,Key_0,Key_6,count)==1) ReturnToMenuMMI(); 
			else if (KeyFound (buf_mmi,Key_0,Key_2,count)==1)
      		{
				Display.point=Cursor.row+Display.prm; 
				Horizont=0; 
				Display.row=0; 
				Display.num=0; 
				Display.flag=1;
				mmi_pass=0; 
				SaveOldPageMMI(); 
				SetDisplayPage(14);
      		} 
		break;
		//01.05.2020 YN -----\\//-----
		case 40:
			//page_str_pass=OK;
			switch (page_temporary)
			{
		   		case 0: case 1: case 2: case 3: case 4: case 5:
		   		case 6: case 7: case 8: case 9: //"   Вычислитель расхода ВРФ    ", //26+27+25
					if(page_str_pass==0) page_screen(0,0,26,1);
					else if(page_str_pass==1) page_screen(0,1,27,2);
					else page_screen(0,15,25,OK);
			    break;
				///////////////////////////////////////////////////////////////
		   		case 10: 						//"         Главное меню         ", //1+18
					if(page_str_pass==0) page_screen(0,0,1,1);
					else page_screen(0,15,18,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 11: case 12:				//"    Меню выбора точки учета   ", //2+18
					if(page_str_pass==0) page_screen(0,0,2,1);
					else page_screen(0,15,18,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 13:						//"  Меню выбора архивной точки  ", //3+18
					if(page_str_pass==0) page_screen(0,0,3,1);
					else page_screen(0,15,18,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 14:						//"     Переменные процесса      ", //4+19
					if(page_str_pass==0) page_screen(0,0,4,1);
					else page_screen(0,15,19,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 15:						//"        Ввод значения         ", //5+21
					if(page_str_pass==0) page_screen(0,0,5,1);
					else page_screen(0,15,21,OK);
				break;
				/////////////////////////////////////////////////////////////				 
				case 16:						//" Изменение конфиг. параметра  ", //6+20
					if(page_str_pass==0) page_screen(0,0,6,1);
					else page_screen(0,15,20,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 17:						//"         Ввод пароля          ", //7+21
					if(page_str_pass==0) page_screen(0,0,7,1);
					else page_screen(0,15,21,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 18:						//"     Неправильный пароль      ", //8+22
					if(page_str_pass==0) page_screen(0,6,8,1);
					else page_screen(0,15,22,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 19:						//"   Просмотр архивной точки    ", //9+19
					if(page_str_pass==0) page_screen(0,0,9,1);
					else page_screen(0,15,19,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 20: case 23:						//"        Меню настроек         ", //10+18
					if(page_str_pass==0) page_screen(0,0,10,1);
					else page_screen(0,15,18,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 21:						//"       Выбор значения         ", //11+18
					if(page_str_pass==0) page_screen(0,0,11,1);
					else page_screen(0,15,18,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 22:						//"Изменение параметра невозможно", //12+22
					if(page_str_pass==0) page_screen(0,6,12,1);
					else page_screen(0,15,22,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 25:						//"Выполн.настр-ка нуля массомера", //13
					if(page_str_pass==0) page_screen(0,6,13,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 26:						//" Внимание! Перекройте расход  ", //14+15+16+23
					if(page_str_pass==0) page_screen(0,5,14,1);
					else if(page_str_pass==1) page_screen(0,6,15,2);
					else if(page_str_pass==2) page_screen(0,7,16,3);
					else if(page_str_pass==3) page_screen(0,15,23,OK);
				break;
				/////////////////////////////////////////////////////////////
				case 27:						//"     Меню выбора прибора      ", //17+18
					if(page_str_pass==0) page_screen(0,0,17,1);
					else page_screen(0,15,18,OK);
				break;
		   	}
			Display.suspend=0;Display.evt=2;count_smb=30;
		break;
	//------------- -----//\\-----
    }
}