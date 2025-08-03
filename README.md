# VC-GenerateRandom
Project using Visual Studio 2022 to develop. Could be used to generate random string or binary data.

No MFC used since it's too fat!

The "Random String" part works as the PHP & ASP.NET version of Random String Generator; the "Random Data" part use WinCrypt API to generate Random Data in given number of element size (BYTE, WORD, DWORD & QWORD) and output the C array initializtion format on the dialogbox. The generated data could also be exported as a raw binary file. Random data could be used as you wish such as the HMAC key, AESpre-defined key or something similiar.

From the codes in this project, one could learn how to assign windows message processing function for a given win32 control, and how to use memory stream api to manipulate a dynamic length buffer.
