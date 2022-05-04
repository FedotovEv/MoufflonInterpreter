﻿
Интерпретатор языка программирования МУФЛОН.
============================================

  Перед   вами     интерпретатор     одного     очень    специфического    языка
программирования   -   Mython.   Транскрипция  этого  слова  в  русский  алфавит
должна   выглядеть  приблизительно  как  "Митон",  но  для  того,  чтобы  слегка
поднять  настроение  всякому  читающему  эти  строки,  по-русски  далее мы будем
называть этот язык программирования так - "Муфлон".

  Сама  реализация  этого   интерпретатора   велась  одновременно  с  чтением  и
проработкой   известной   "книги   дракона"   -   Ахо,   Ульман,  пр.  "Принципы
разработки   компиляторов".   Так   что,   в   основном,   данный  проект  носит
учебно-образовательный    характер.   Практическое   применение   такому   очень
ограниченному   языку   найти   будет  весьма  трудно.  Однако,  в  конце  этого
документа    несколько    способов    такового    будет    всё    же    указано.  

  Что  же такое  Муфлон?  Как следует  даже  из  названия,  это некоторый, очень
сильно    урезанный    вариант    широко   распространённого   интерпретируемого
скриптового   языка   Питон.   Из  него  исключена  масса  конструкций,  которые
могли      бы      затруднить      проектирование     "теоретически     ясного",
архитектурно-чистого    транслятора    на   основе   стандартного   абстрактного
синтаксического    дерева.    В   частности,   в   нём   поддерживается   только
целочисленная  арифметика  (дробные  числа  с  плавающей  точкой отсутствуют), а
также  исключены  всякие  операторы  циклов.  Описанию  самого  этого языка, его
лексики и синтаксиса, будет посвящена отдельная идущая ниже глава.

Описание языка программирования Муфлон.
---------------------------------------

  Муфлон     -     объектно-ориентированный      язык     программирования     с
поддержкой классов и наследования. Все методы классов — виртуальные.

* Числа.  
  В   языке   Муфлон   используются   только   целые   числа.   С   ними   можно
выполнять  обычные  арифметические  операции:  сложение,  вычитание,  умножение,
целочисленное деление.

* Строки.  
  Строковая  константа  в   Муфлоне   —   это   последовательность  произвольных
символов,  размещающаяся  на  одной  строке  и  ограниченная  двойными кавычками
"  или  одинарными  '.  Поддерживается  экранирование  спецсимволов  '\n', '\t',
'\'' и '\"'.

  Примеры строк в Муфлоне:

		"hello"  
		'world'  
		'long string with a double quote " inside'  
		"another long string with a single quote ' inside"  
		"string with a double quote \" inside"  
		'string with a single quote \' inside'  

  '', "" — пустые строки.  
  Строки в языке Муфлон — неизменяемые.
  
* Логические константы и None.

  Кроме   строковых  и   целочисленных   значений   язык   Муфлон   поддерживает
логические             значения             True             и            False.
Есть  также  специальное  значение  None,  выполняющее функцию  пустого значения
(пустого   указателя).   Обратите   внимание,  логические  константы  пишутся  с
большой буквы.

* Комментарии.

  Муфлон  поддерживает  однострочные  комментарии,  начинающиеся  с  символа  \#.
Весь     следующий    текст    до    конца    текущей    строки    игнорируется.
\# внутри строк считается обычным символом.

		# это комментарий  
		x = 5 #это тоже комментарий  
		# в следующей строке # - обычный символ  
		hashtag = "#природа" 

* Идентификаторы.

  Идентификаторы  в  Муфлоне   используются  для  обозначения  имён  переменных,
классов   и   методов.  Правила  именования  идентификаторов  такие  же,  как  в
большинстве   других   языков   программирования:  начинаются  со  строчной  или
заглавной   латинской   буквы   либо  с  символа  подчёркивания.  Потом  следует
произвольная   последовательность,   состоящая   из   цифр,   букв   и   символа
подчёркивания.  
  Примеры   правильных   идентификаторов:   x,   _42,   do_something,   int2str.
Примеры                       неправильных                      идентификаторов:
4four   —   начинается   с   цифры;   one;two  —  содержит  символ,  который  не
относится к цифрам, буквам или знакам подчёркивания.

* Классы.

  В Муфлоне можно  определить  свой  тип,  создав  класс.  Класс  имеет  поля  и
методы,    поля    не    надо    объявлять    заранее.    Объявление    поля   -
первое  его  применение.  Объявление   класса   начинается  с   ключевого  слова
class, за которым следует идентификатор имени и объявление методов класса.

  Пример класса «Прямоугольник»:

      class Rect:
        def __init__(w, h):
          self.w = w
          self.h = h

        def area():
           return self.w * self.h 

* Важные особенности классов в Муфлоне:

  Специальный    метод    __init__     играет    роль    конструктора    —    он
автоматически       вызывается       при       создании      нового      объекта
класса.       Метод       __init__       может       отсутствовать.      Неявный
параметр     всех     методов    —    специальный    параметр    self,    аналог
указателя    this    в    C++.    Параметр    self    ссылается    на    текущий
объект   класса.   Поля   не   объявляются   заранее,  а  добавляются  в  объект
класса   при   первом   присваивании.   Поэтому   обращения   к   полям   класса
всегда    надо    начинать   с   self.,   чтобы   отличать   их   от   локальных
переменных. Все поля объекта — публичные.

  Новый объект ранее  объявленного  класса  создаётся  указанием  имени  класса,
за которым в скобках идут параметры, передаваемые методу __init__.

    r = Rect(10, 5) 

  В этой программе  создаётся  новый  объект  класса  Rect.  При  вызове  метода
__init__  параметр  w  будет  иметь  значение  10,  а  параметр h — 5. Созданный
прямоугольник будет доступен в переменной r.

* Типизация.

  Муфлон — это  язык  с  динамической  типизацией.  В  нём тип каждой переменной
определяется   во  время  исполнения  программы  и  может  меняться  в  ходе  её
работы.   Поэтому  какие-либо  механизмы  явного  указания  типа  переменной  не
требуются.

  Пример:

		x = 4  # переменная x связывается с целочисленным значением 4  
		# следующей командой переменная x связывается со значением 'hello'  
		x = 'hello'  
		y = True  
		x = y 

* Операции.

  В Муфлоне определены:
  +  Арифметические операции для целых чисел, деление выполняется нацело.
  Деление на ноль вызывает ошибку времени выполнения.
  + Операция конкатенации строк, например: s = 'hello, ' + 'world'.
  + Операции сравнения строк и целых чисел ==, !=, <=, >=, <, >; сравнение
  строк выполняется лексикографически.
  + Логические операции and, or, not.
  + Унарный минус.
  + Приоритет операций (в порядке убывания приоритета):
  + Унарный минус.
  + Умножение и деление.
  + Сложение и вычитание.
  + Операции сравнения.
  + Логические операции.

  Порядок вычисления выражений может быть изменён скобками:

      print 2 + 3 * 4   # выведет 14
      print (2 + 3) * 4 # выведет 20 

    В  Муфлоне  операция  сложения  кроме  чисел  и  строк  применима  к  объектам
  классов со специальным методом \_\_add__:

      class Fire:
        def __init__(obj):
          self.obj = obj
      
        def __str__():
          return "Burnt " + str(self.obj)
      
      class Tree:
        def __str__():
          return "tree"
      
      class Matches: # Спички
        # операция сложения спичек с другими объектами превращает их в огонь
        def __add__(smth):
          return Fire(smth)
      
      result = Matches() + Tree()
      print result             # Выведет Burnt tree
      print Matches() + result # Выведет Burnt Burnt tree 

    Операции  сравнения  применяются  не  только  к  числам  и  строкам,  но  и  к
  объектам   классов,   имеющих   методы   \_\_eq__   (проверка  «равно»)  и  \_\_lt__
  (проверка   «меньше»).   Используя   эти   методы,   реализуются  все   операции
  сравнения.

      class Person:
        def __init__(name, age):
          self.name = name
          self.age = age
        def __eq__(rhs):
          return self.name == rhs.name and self.age == rhs.age
        def __lt__(rhs):
          if self.name < rhs.name:
              return True
          return self.name == rhs.name and self.age < rhs.age
      
      print Person("Ivan", 10) <= Person("Sergey", 10) # True
      print Person("Ivan", 10) <= Person("Sergey", 9)  # False 

* Функция str.

  Функция str преобразует  переданный  ей  аргумент  в  строку.  Если аргумент —
объект  класса,  она  вызывает  у  него  специальный  метод \_\_str__ и возвращает
результат.  Если  метода  \_\_str__  в  классе  нет,  функция возвращает строковое
представление адреса объекта в памяти.

  Примеры:

      str('Hello') вернёт строку Hello;
      str(100500) вернёт строку 100500;
      str(False) вернёт строку False;
      str(Rect(3, 4)) вернёт адрес объекта в памяти, например 0x2056fd0.
      
      Пример класса с методом __str__:
      class Rect(Shape):
        def __init__(w, h):
          self.w = w
          self.h = h
      
        def __str__():
          return "Rect(" + str(self.w) + 'x' + str(self.h) + ')' 

  Выражение str(Rect(3, 4)) вернёт строку Rect(3x4).

* Команда print.

  Специальная   команда   print    принимает   набор   аргументов,   разделённых
запятой,  печатает  их  в  стандартный  вывод  и  дополнительно  выводит перевод
строки. Посмотрите на этот код:

      x = 4
      w = 'world'
      print x, x + 6, 'Hello, ' + w 

  Он выведет:  
  4 10 Hello, world 

  Команда  print вставляет  пробел  между  выводимыми  значениями.  Если  ей  не
передать  аргументы,  она  просто  выведет  перевод  строки. Чтобы преобразовать
каждый  свой  аргумент  в  строку,  команда  print  вызывает  для  него  функцию
str.  Таким  образом,  команда  print  Rect(20,  15)  выведет  в  stdout  строку
Rect(20x15).

* Условный оператор.

  В Муфлоне есть условный оператор. Его синтаксис:

      if <условие>:
        <действие 1>
        <действие 2>
        ...
        <действие N>
      else:
        <действие 1>
        <действие 2>
        ...
        <действие M> 

  <условие>   —   это   произвольное  выражение,  за  которым  следует  двоеточие.
Если  условие  истинно,  выполняются  действия  под  веткой  if,  если  ложно  —
действия   под   веткой   else.  Наличие  ветки  else  необязательно.  <условие>
может   содержать  сравнения,  а  также  логические  операции  and,  or  и  not.
Условие  будет  истинным  или  ложным  в  зависимости  от  того, какой тип имеет
вычисленное выражение.

  Если  результат  вычисления   условия   —   значение   логического  типа,  для
проверки истинности условия используется именно оно.

  Примеры:

      if x > 0:
      if s != 'Jack' and s != 'Ann':

  Если  результат  вычисления   условия   —   число,  условие  истинно  тогда  и
только тогда, когда это число не равно нулю.

  Если  результат  вычисления   условия   —  строка,  условие  истинно  тогда  и
только   тогда,   когда   эта  строка  имеет  ненулевую  длину.  Если  результат
вычисления   условия   —   объект   класса,   условие  истинно.  Если  результат
вычисления условия — None, условие ложно.

  Действия в ветках  if  и  else  набраны  с  отступом  в два пробела. Вообще, в
языке   Муфлон  команды  объединяются  в  блоки  отступами.  Один  отступ  равен
двум    пробелам.    Отступ    в    нечётное   количество   пробелов   считается
некорректным.

  Сравните:

      if x > 0:
        x = x + 1
      print x
      
      if x > 0:
        x = x + 1
        print x 

  Первая команда print  x  будет  выполняться  всегда,  вторая  —  только если x
больше 0. Вложенность условий может быть произвольной:

      if x > 0:
        if y > 0:
          print "Эта строка выведется, если x и y положительные"
      else:
        print "Эта строка выведется, если x <= 0" 

* Наследование.

  В  языке  Муфлон  у  класса  может  быть  один  родительский  класс.  Если  он
есть,  он  указывается  в  скобках  после  имени  класса и до символа двоеточия.
В примере ниже класс Rect наследуется от класса Shape:

      class Shape:
        def __str__():
          return "Shape"
      
        def area():
          return 'Not implemented'
      
      class Rect(Shape):
        def __init__(w, h):
          self.w = w
          self.h = h
      
        def __str__():
          return "Rect(" + str(self.w) + 'x' + str(self.h) + ')'
      
        def area():
          return self.w * self.h 

  Наследование в языке  Муфлон,  как  и  ожидается,  приводит  к  тому,  что все
методы   родительского  класса  становятся  доступны  классу-потомку.  При  этом
все   методы   публичные  и  виртуальные.  Например,  код  ниже  выведет  Hello,
John:

      class Greeting:
        def greet():
          return "Hello, " + self.name()
      
        def name():
          return 'Noname'
      
      class HelloJohn(Greeting):
        def name():
          return 'John'
      
      greet_john = HelloJohn()
      print greet_john.greet() 

* Методы.

  Как вы могли заметить, методы в Муфлоне имеют синтаксис:

      def <имя метода>(<список параметров>):
        <действие 1>
        <действие 2>
        ...
        <действие N> 

  Ключевое  слово def  располагается  с  отступом  в  два  пробела  относительно
класса.  Инструкции,  составляющие  тело  метода,  имеют  отступ  в  два пробела
относительно  ключевого  слова  def.  Как  и  в случае полей класса, обращения к
полям и методам текущего класса надо начинать с self.:

      class Factorial:
        def calc(n):
          if n == 0:
            return 1
          return n * self.calc(n - 1)
      
      fact = Factorial()
      print fact.calc(4) # Prints 24 

  Этот    пример    также     показывает     поддержку     рекурсии,     которая
компенсирует    отсутствие    циклов   в   языке.   Команда   return   завершает
выполнение   метода   и   возвращает   из   него   результат  вычисления  своего
аргумента.   Если   исполнение   метода   не  достигает  команды  return,  метод
возвращает None.

* Семантика присваивания.

  Как сказано выше,  Муфлон  —  это  язык  с  динамической типизацией, и все его
переменные   являются   указателями   на   области   памяти,   где  хранятся  их
действительные  на  момент  исполнения  инструкции  значения.  Поэтому  операция
присваивания  имеет  семантику  не  копирования  значения  в  область  памяти, а
изменения    адреса,   на   который   указывает   переменная.   Как   следствие,
переменные  только  ссылаются  на  значения,  а  не  содержат  их  копии. Пустой
(нулевой)   указатель   —   значение   None.   Код   ниже  выведет  2,  так  как
переменные x и y ссылаются на один и тот же объект:

      class Counter:
        def __init__():
          self.value = 0
      
        def add():
          self.value = self.value + 1
      
      x = Counter()
      y = x
      x.add()
      y.add()
      print x.value 

* Прочие ограничения.

  Результат  вызова  метода   или   конструктора   в   Муфлоне   —  терминальная
операция.  Её  результат  можно  присвоить  переменной  или  использовать в виде
параметра   функции   или   команды,   но   обратиться   к   полям   и   методам
возвращённого объекта напрямую нельзя:

      # Так нельзя
      print Rect(10, 5).w
      # А вот так можно
      r = Rect(10, 5)
      print r.w 

Способ запуска программ интерпретатором.
----------------------------------------

  Для исполнения программы  на  языке  Муфлон  описываемым  интерпретатором  она
должна быть подвергнута последовательно следующим стадиям преобразования:

1. Лексический разбор.
2. Синтаксический  анализ.  В  результате  его  будет  построено  синтаксическое
дерево программы.
3. Непосредственно исполнение дерева, построенного на предыдущем шаге.

Так всё это выглядит, собственно, в коде:

    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context(output);
    runtime::Closure closure;
    program->Execute(closure, context);

  Входному   лексическому   анализатору    parse::Lexer   программа   передаётся
через     какой-либо     стандартный     поток,     приводимый     к    istream.
Результат   работы   лексического   разборщика  передаётся  для  синтаксического
анализа  через  параметр  функции  ParseProgram,  возвращающей  объект корневого
узла  абстрактного  синтаксического  дерева  программы.  Наконец,  вызывая метод
Execute  этого  узла  (он  есть  у  каждого узла построенного дерева), запускаем
программу  на  исполнение.  Аргументами  этого  метода  служат  таблица символов
программы  closure  и  вспомогательная  переменная  context,  хранящая состояние
исполнительской   среды.   Этот   контекст   служит   для   хранения   некоторых
параметров, используемых для исполнения отдельных операторов языка.

  Таким  оператором,  является,  в  частности,  команда  print.  Она  направляет
весь   свой   вывод   в   какой-либо   выходной  поток,  приводимый  к  ostream.
Конкретный поток, который будет использовать print, хранится в контексте.
  
  Создать  нужный  контекст   можно  с  помощью  класса  runtime::SimpleContext,
который   представляет   один   из   видов   такого   контекста,  совместимый  с
методами   Execute   синтаксического   дерева.  Его  конструктор  принимает  два
аргумента   -   выходной   поток   для   print  и  необязательный  указатель  на
функцию,  которая  будет  служить  каналом  обмена  информацией для специального
объекта __external (в приведённом примере он не показан).

Возможные области практического применения языка.
-------------------------------------------------

  Как  следует  из   вышеприведённого   описания,   возможности   языка   весьма
скудные.  Отсутствие  поддержки  дробных  чисел  делает  невозможным  выполнение
математических    вычислений   обычным   способом   (это   возможно   только   с
применением   специальных   приёмов,  таких  как  введение  специального  класса
для   чисел   с   плавающей  точкой).  Кроме  того,  отсутствие  в  языке  явных
операторов   циклов   или,   хотя   бы,  возможности  непосредственной  передачи
управления  в  определённую  точку  кода  (какой-либо  аналог  оператора  goto),
приводит   к   крайней   сложности   реализации   алгоритмов,  содержащих  любые
повторяющиеся   действия.   Для   этого   можно   применять  рекурсивные  вызовы
функций,   но   это   требует   больших  накладных  вычислительных  расходов  со
стороны   интерпретатора   и  приводит  к  быстрому  исчерпанию  ёмкости  стека,
поэтому  реализовывать  таким  образом  циклы  с  большим  числом  повторений не
удаётся.

  Всё это  очерчивает возможную  на  данный  область  применения  этого  языка -
встроенный  язык  простых  сценариев  для  какой-либо  иной программной системы.
Но  для  этого  нужно  предусмотреть  механизмы  взаимодействия интерпретатора с
объемлющим   комплексом,   в   который   он   будет   включён.   Описанию  таких
механизмов посвящён следующий небольшой раздел.

Взаимодействие интерпретатора языка Муфлон с внешней средой.
------------------------------------------------------------

  Взаимодействие     интерпретатора    с     внешней     программой     возможно
двумя   способами:   через   создание   нестандартного  потока  вывода  и  через
обращение    к   полям   специального   системного   объекта   __external.   Они
оба     хранятся    в    контексте    исполнения    программы.    Нестандартный,
пользовательский    выходной    поток    должен    быть   приводим   к   ostream
и        будет        получать       весь       вывод       команды       print.
Метод-получатель     информации     внутри     такого     потокового    объекта,
получая   данные   из   программы  на  Муфлоне,  может  в  соответствии  с  ними
предпринимать  любые  действия  по  своему  усмотрению.  Простой  пример  такого
взаимодействия  приведён  в  приложенной  к  проекту  демонстрационной программе
NonStdPrint.cpp.

  Второй     канал     обмена      информацией     с     Муфлон-программой     -
использование    специального    объекта    __external.    Если   при   создании
SimpleContext   использовать  трёхоперандный  вариант  конструктора,  то  третий
операнд   будет   указывать   на   функтор   (любой   вызываемый   объект)  типа
LinkageFunction, определённый так:

    namespace runtime
    {
        using LinkageReturn = variant<int, string>;
        using LinkageFunction = function<LinkageReturn(LinkCallReason,
                                const string&, const vector<string>&)>;
        .................................
    }

Эта функция принимает три аргумента:

1. Тип  операции  -  перечисление LinkCallReason - оно указывает  причину вызова
функции:  этот  параметр  принимает  значение CALL_REASON_WRITE_FIELD при записи
в  какое-либо  поле  объекта,  и  CALL_REASON_READ_FIELD  при  чтении  из  него.

2. Имя поля

3. Массив    (вектор)   строк,  образованный   из  значений,  которые  выступают
входными   аргументами  операции.  Каждая  строка  вектора  будет  формироваться
из   соответствующего   по  номеру  входного  значения  по  тому  же  алгоритму,
который  применяется  описанной  ранее  функцией  str.  При  выполнении операции
записи   в   поле   такой  аргумент  только  один  -  собственно,  то  значение,
которое  должно  быть  присвоено  полю.  Оно  будет  являться  нулевым элементом
вектора.  Операция  чтения  из  поля  вовсе не имеет входных аргументов, поэтому
для неё вектор будет пуст.

  В  процессе  исполнения   программы   при  любом  обращении  к  полям  объекта
__external   (чтение   или   присваивание  любых  его  полей)  будет  вызываться
указанный   через   конструктор   контекста   SimpleContext  функтор,  аргументы
которого заполняются описанным выше способом.

  При  осуществлении   операции  чтения   из   поля   объекта   функция   должна
вернуть   некоторое   значение.   Это   значение   принадлежит   к   вариантному
библиотечному  типу  STL  (std::variant)  и  может  содержать либо целочисленную
величину,   либо   строку.   Возвращенный   таким   способом   результат   будет
считаться    в    программе   текущим   значением   самого   этого   поля.   При
выполнении  записи  возвращаемое значение игнорируется.
  Пример   использования    этого   механизма    приведён   в   демонстрационной
программе ExternalObjectDemo.cpp.

Возможные доработки, дополнения и планы на будущее.
---------------------------------------------------

  Бедность  синтаксиса   языка  очень  затрудняет его  практическое  применение.
Поэтому  главное  -  расширить  его  до  минимально  приемлемых  границ.  Прежде
всего  -  добавление  поддержки  дробных  чисел  с плавающей точкой и введение в
язык   операторов  циклов  и  безусловного  перехода  (с  ограничениями). 

  Второе   очень   тяжёлое  ограничение   -   отсутствие   в   языке   поддержки
массивов.   На   данный   момент   все   переменные   в   программе  на  Муфлоне
только   скалярные.   При   таком   положении   вещей   ситуацию   не   исправит
даже     наличие     циклов.    Поэтому    обязательно    следует    ввести    в
язык    массивы    и    косвенное/индексное    обращение    к    памяти.   После
этого     программирование    на    нём    станет    вполне    комфортным,    во
всяком  случае, для составления скриптов и простых сценариев.

Компиляция и сборка проекта.
----------------------------

  Программа  может  быть   собрана   любым   компилятором   C++,  поддерживающим
стандарт    C++17. В   целях    облегчения    сборки    к    проекту   приложены
сборочный    cmake-скрипт    и    уже    готовый    проект    ("решение")    для
Visual   Studio.   Каких-либо   внешних  нестандартных  зависимостей  проект  не
имеет.
