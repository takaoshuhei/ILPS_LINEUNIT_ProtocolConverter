using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Windows;

public class Client
{
    [DllImport("libember_slim.dll")]

    extern static int main();

    static void Main()
    {
        if (main() == 0)
        {
            Console.WriteLine("èàóùèIóπ");
            Console.ReadLine();
        }

    }
}