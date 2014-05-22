using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Tmc.Robotics;

namespace ConsoleApplication2
{
    class Program
    {
        static void Main(string[] args)
        {
            IConveyor serialConveyor = new SerialConveyor();
            var dict = new Dictionary<string, string>();
            dict.Add("Name", "SerialConveyor");
            dict.Add("PortName", "COM10");

            serialConveyor.SetParameters(dict);
            serialConveyor.Initialise();

            
            for(;;)
            {
                switch(Console.ReadLine())
                {
                    case "f":
                        serialConveyor.MoveForward();
                        break;
                    case "b":
                        serialConveyor.MoveBackward();
                        break;
                }
            }
            
        }
    }
}
