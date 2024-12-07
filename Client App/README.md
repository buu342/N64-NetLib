# NetLib Browser

This program will help you find servers to connect to, and then act as a USB and internet communication tether for the N64. 

### Compilation

This program uses [wxWidgets](https://www.wxwidgets.org/) to allow for a cross platform GUI. 

<details><summary>Windows</summary>
<p>

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_msw_install.html). **Make sure you compile with Sockets enabled.** If you are able to compile the sockets samples (located in wxWidgets/samples/sockets), then you have succeeded.

If it's not yet defined, make sure you set the `WXWIN` environment variable to point to your wxWidgets folder.

If you have successfully installed wxWidgets, then simply open `NetLibBrowser.vcxproj` with Visual Studio 2019 or higher, and compile.

</p>
</details>

<details><summary>Linux</summary>
<p>

Start by installing or building wxWidgets using this [guide](https://docs.wxwidgets.org/trunk/plat_gtk_install.html). **Make sure you compile with Sockets enabled.** If you are able to compile the sockets samples (located in wxWidgets/samples/sockets), then you have succeeded.

If you have successfully installed wxWidgets, then simply run `make` to compile.

### Credits

* Brad Conte for the [SHA256 library](https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c) used for ROM hashing.