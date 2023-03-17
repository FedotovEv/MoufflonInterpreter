
Демонстрационная программа для "файловой" втыкалы MythonFilePlugin.
-------------------------------------------------------------------

Размещённая здесь программа представялет собой модульный тест и одновременно иллюстрацию по
применению втыкалы, обеспечивающей некоторые файловые операции из программ на Муфлоне.
Сама втыкала находится в папке MythonFilePlugin главного репозитория проекта.

Данная программа создаёт файл testfile.txt в текущем каталоге активного диска, после чего
проделывает с ним некоторые операции записи, последующего чтения и, наконец, удаления это файла.
При этом проверяется корректность некоторых выполняемых операций, и в случае обнаружения
ошибок вы получите "утвердительное исключение" (assert) с соответствующей диагностикой.
При безошибочном выполнении программа завершается молча, без всякого вывода чего-либо в консоль.