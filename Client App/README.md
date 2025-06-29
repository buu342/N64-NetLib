# NetLib Browser

This program will help you find servers to connect to, and then act as a USB and internet communication tether for the N64. 

### Compilation

This program uses [wxWidgets](https://www.wxwidgets.org/) to allow for a cross platform GUI. 

<details><summary>Windows</summary>
<p>
    
Because NetLib Browser is designed with Windows XP compatibility in mind, I use Visual Studio 2019 with the old XP SDK (`v141_xp`) to compile the solution. If you are not interested in supporting XP, then feel free to use a later version of the IDE and SDK. 

Download the [wxWidgets source code ZIP](https://wxwidgets.org/downloads/), and extract it. If you are targeting Windows XP, use wxWidgets 3.2.8, otherwise use the latest. Go to the `build/msw` folder and open the solution file for your Visual Studio version. Since I am using VS 2019, open `wx_vc16.sln` (as Visual Studio 2019 refers to version 16). Once the solution loads, you will need to edit all the projects options to use `/MT` for code generation, as wxWidgets compiles with `/MD` by default. Afterwards, if you want XP compatibility then make sure you set the Platform Toolset to `v141_xp`. After those steps are completed, compile the wxWidgets static libraries using your preferred setup (debug/release 32/64). Once wxWidgets is finished compiling, make sure that the `WXWIN` environment variable is set on your system to point to the wxWidgets directory (if it isn't, do so). If you are able to compile the samples (after changing them to `/MT` and their platform toolkit if needed), then you are good to go.

After that, download [UNFLoader](https://github.com/buu342/N64-UNFLoader/), open `UNFLoader/FlashcartLib_Static.vcxproj`, and make sure it had the exact same configuration (debug/release 32/64). This will compile a `.lib`, which you should rename to `Flashcart.lib` and place inside the `Include` folder in the `Client App` folder. If you want compile a debug version of the NetLib Browser, then build a debug version of the Flashcart library and rename it to `Flashcart_d.lib` before adding it to the include folder. If you want a 64-Bit executable, then rename the file to `Flashcart_x64.lib` (append `_d` to the end if you build a 64-bit debug library).

If you have successfully installed wxWidgets and built the flashcart library, then simply open `NetLibBrowser.vcxproj` and compile (again, make sure you have the same configuration).

</p>
</details>

<details><summary>MacOS</summary>
<p>

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_gtk_install.html). During the configuration step, I used `../configure --with-opengl --disable-shared --disable-sys-libs`. If you are able to compile the sockets samples (located in `wxWidgets/samples/sockets`), then you have succeeded. 

After that, download [UNFLoader](https://github.com/buu342/N64-UNFLoader/), go to the `UNFLoader` folder, and compile the flashcart library with `make static`. This will produce a `flashcart.a`, which you should now place inside the `Include` folder in the `Client App` folder. You can compile a debug version of the library with `make static DEBUG=1`.

If you have successfully installed wxWidgets and built the flashcart library, then simply run `make` to compile. You can compile a debug version with `make DEBUG=1`.

</p>
</details>

<details><summary>Linux</summary>
<p>

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_gtk_install.html). During the configuration step, I used `../configure --with-opengl --disable-shared`. If you are able to compile the sockets samples (located in `wxWidgets/samples/sockets`), then you have succeeded. 

After that, download [UNFLoader](https://github.com/buu342/N64-UNFLoader/), go to the `UNFLoader` folder, and compile the flashcart library with `make static`. This will produce a `flashcart.a`, which you should now place inside the `Include` folder in the `Client App` folder. You can compile a debug version of the library with `make static DEBUG=1`.

If you have successfully installed wxWidgets and built the flashcart library, then simply run `make` to compile. You can compile a debug version with `make DEBUG=1`.

</p>
</details>

### Credits

* Brad Conte for the [SHA256 library](https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c) used for ROM hashing.
* The [ASIO C++ Library](https://think-async.com/Asio/) used to replace wxWidgets' buggy as hell `wxDatagramSocket`.