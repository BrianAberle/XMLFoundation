#pragma once
namespace InterfaceMyType
{
   public interface class IMyType
      {
		 // Define a property with a public getter
         property Platform::String^ Name
         {
               Platform::String^ get();
         }

		 // A method
         void MyMethod();
      };
}
