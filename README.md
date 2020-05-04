# hide-and-seek

PoC for hiding processes from Windows Task Manager by manipulating the graphic interface

# WORK IN PROGRESS

### Description

Window messages can be used to communicate user actions to other processes. This proof of concept manipulates windows elements from the Windows Task Manager process to hide another process. 

*The same technique can be used to hide Windows services, registry keys from Regedit or other elements*. 

The first hurdle to overcome when creating this is *__UIPI__*:

> User Interface Privilege Isolation (UIPI) is a technology introduced in Windows Vista and Windows Server 2008 to combat [shatter attack exploits](https://en.wikipedia.org/wiki/Shatter_attack). UIPI's Mandatory Integrity Control prevents processes with a lower integrity level from sending messages to higher integrity level processes (*except for a very specific set of UI messages*).

Because of UIPI, nowadays this technique requires running the process with elevated privileges. In a real world scenario, UAC [can be bypassed](https://github.com/hfiref0x/UACME) and [UIPI can be disabled](https://nsylvain.blogspot.com/2008/01/integrity-drop-or-how-to-disable-uipi.html).

When creating this PoC, [WinSpy++](https://www.autohotkey.com/boards/viewtopic.php?f=6&t=28220) was tremendously useful to figure out the hierarchy of various Windows elements of Task Manager. Kudos to James Brown (WinSpy++ developer) and other people who worked on creating and maintaining this great tool!
 
![WinSpy++](img/winspy.png)

One other difficulty is that one process cannot easily read elements from another process's GUI elements, like list items. For this, I had to use a [workaround](http://www.codeproject.com/Articles/5570/Stealing-Program-s-Memory):
* First allocate memory inside the target process - Task Manager
* Send a message to the Task Manager process to read te process list elements _to that memory block_
* Read the memory of the Task Manager and get the process list elements

The PoC also does a few things to prevent the re-appearance of the hidden process:

* Pauses the refresh of the process list
* Disables the _Update speed_ menu
* Disables the _Refresh now_ menu option

### Usage

* Start *Windows Task Manager* ➝ Details tab.
* Launch a test process (```calc.exe```)
* Launch the PoC from a console with admin rights

![Usage](img/howto.png)

* Note that after a first run the _Refresh now_ will be disabled and also the _Update speed_ will be paused. To re-test, Task Manager needs to be restarted!

### Compile

Compiled with **Microsoft Visual Studio Community 2019, Version 16.5.3** as a **64-bit application** - This is very important since Task Manager is a 64-bit application and reading/writing from its memory from a 32-bit process would be a lot more difficult.

Tested on Microsoft Widows 8.1 Enterprise, Version 6.3.9600, 64-bit.

### Contribute

Please send any ideas to [reversinghub@gmail.com](mailto:reversinghub@gmail.com). If you want to contribute to the code, send a PR request. 

### References
 * [WinSpy++](https://www.autohotkey.com/boards/viewtopic.php?f=6&t=28220)
 * [User Interface Privilege Isolation](https://en.wikipedia.org/wiki/User_Interface_Privilege_Isolation)
 * [The integrity drop - or - How to disable UIPI take 2](https://nsylvain.blogspot.com/2008/01/integrity-drop-or-how-to-disable-uipi.html)
 * [hfiref0x/UACME](https://github.com/hfiref0x/UACME)
 * [Shatter attack](https://en.wikipedia.org/wiki/Shatter_attack)
 * [Stealing a program's memory](http://www.codeproject.com/Articles/5570/Stealing-Program-s-Memory)
