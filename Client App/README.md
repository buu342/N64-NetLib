# NetLib Browser

This program will help you find servers to connect to, and then act as a USB and internet communication tether for the N64. 

### Compilation

This program uses [wxWidgets](https://www.wxwidgets.org/) to allow for a cross platform GUI. 

<details><summary>Windows</summary>
<p>

To build, I use Visual Studio 2019, I specifically used the `v141_xp` toolset for WinXP compatibility, release builds, and built everthing in 32-bit. You are free to use a later VS/toolset/debug build/64-bit, just make sure you keep it consistent throughout these instructions or you will get linker errors.

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_msw_install.html). Since I use VS2019, I opened `build/msw/wx_vc16.sln`, changed /MD to /MT in the solution properties, changed the toolset to `v141_xp`, and then built a 32-bit release. Once it's done, check if the `WXWIN` environment variable is set, and if it isn't, make one to point to your wxWidgets folder.

After that, download [UNFLoader](https://github.com/buu342/N64-UNFLoader/), open `UNFLoader/FlashcartLib_Static.vcxproj`, and make sure it had the exact same configuration (32-bit release, /MT, and built with `v141_xp`). This should compile a `Flashcart.lib`, which you should now place inside the `Include` folder in the `Client App` folder.

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