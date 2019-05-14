using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using ATLDllLib;

namespace ATLTest
{
    class Program
    {
        static void Main(string[] args)
        {
            //调用ATL COM组件
            AtlClass atl = new AtlClass();
            atl.init(0);
            atl.add(1, 2);
            int result = atl.result;
            
        }
    }
}
