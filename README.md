# sql_PROXY_server

<h1>Сборка</h1>
<hr>
<pre>brew install boost</pre>
В папке проекта
<pre>cmake .</pre>
<pre>make</pre>
<pre>./proxy</pre>
запуск postgresql-сервера<br>
подключаю pgAdmin4 к 127.0.0.1:5431(1 Потому что этот порт прослушывает proxy)<br>
Смотрю log.txt в папке проекта<br>
<hr>

<h1>Тестовое задании компании dataArmor</h1>

Разработать на C++ TCP прокси-сервер для СУБД с возможностью логирования всех SQL запросов, проходящих через него.
В качестве СУБД на выбор можно использовать:

-MySQL (а также клоны — MariaDB/Percona);

-PostgreSQL;

-MS SQL Server.

Для выполнения тестового задания можно использовать одно из следующих средств (на выбор):

-ACE (AdaptiveCommunicationEnvironment);

-boost.asio;

-WinSock (желательно IOCP);

-Berkley sockets (select/poll/epoll);

-libev/libevent.

Прочих зависимостей быть не должно.

Требования к прокси

- Прокси должен смочь обрабатывать большое количество одновременных соединений.
- В лог должны попадать только SQL-запросы.
- Код должен корректно обрабатывать ошибки в протоколе.
- Задание должно быть прислано вместе с файлами проекта (для Visual Studio или MakeFiles).
- Код должен быть оформлен в виде проекта Visual Studio 19/22 для Windows или в виде файлов сборки CMake/Make для Linux.
TIPs
- В рамках тестового задания поддержка шифрованных соединений не требуется.
Цель выполнения тестового задания – проверка профессиональных навыков кандидатов на вакантную позицию. Написанный Вами код не будет использоваться в продуктах компании или передан третьим лицам. Результат выполнения просим выслать архив папкой.
