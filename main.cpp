//
//  main.cpp
//  JarFromMemory
//
//  Created by Brandon Thomas on 2016-02-14.
//  Copyright © 2016 BT. All rights reserved.
//


#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

#include "JVM.hpp"

std::vector<unsigned char> readClass(JVM* vm, InputStream* in)
{
    ByteArrayOutputStream baos = ByteArrayOutputStream(vm);
    int value = -1;
    
    while((value = in->read()) != -1)
    {
        baos.write(value);
    }
    return baos.toByteArray();
}

int main(int argc, const char * argv[])
{
    std::vector<unsigned char> jar;
    std::fstream file("Eos.jar", std::ios::in | std::ios::binary);
    
    if (file)
    {
        file.seekg(0, std::ios::end);
        jar.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&jar[0]), jar.size());
    }
    
    
    const char* jvmArgs[] = {"-Djava.compiler=NONE", "-Djava.class.path=."};
    JVM jvm(sizeof(jvmArgs) / sizeof(jvmArgs[0]), jvmArgs);
    
    ByteArrayInputStream bais = ByteArrayInputStream(&jvm, jar.data(), jar.size());
    JarInputStream jis = JarInputStream(&jvm, &bais);
    
    JarEntry* entry = nullptr;
    
    while((entry = jis.getNextJarEntry()) != nullptr)
    {
        std::string ext = ".class";
        std::string name = entry->getName();
        
        if ((name.length() > ext.length()) && (name.rfind(ext) == name.length() - ext.length()))
        {
            auto ReplaceAll = [&](std::string str, const std::string& from, const std::string& to) -> std::string {
                size_t start_pos = 0;
                while((start_pos = str.find(from, start_pos)) != std::string::npos)
                {
                    str.replace(start_pos, from.length(), to);
                }
                return str;
            };
            
            std::vector<unsigned char> bytes = readClass(&jvm, &jis);
            std::string canonicalName = ReplaceAll(ReplaceAll(name, "/", "."), ".class", "");
            
            jclass cls = jvm.DefineClass(ReplaceAll(name, ".class", "").c_str(), nullptr, reinterpret_cast<jbyte*>(bytes.data()), bytes.size());
            if (cls)
            {
                std::cerr<<"Defined: "<<canonicalName<<" Size: "<<bytes.size()<<"\n";
            }
            else
            {
                std::cerr<<"Failed to define: "<<canonicalName<<"  Size: "<<bytes.size()<<"\n";
            }
        }
        
        
        delete entry;
        entry = nullptr;
    }
    
    jis.close();
    bais.close();
    
    
    jclass mainClass = jvm.FindClass("eos/Main");
    std::cout<<"Main Class: "<<mainClass<<"\n";
    
    jobjectArray arr = jvm.NewObjectArray(1, jvm.FindClass("java/lang/String"), nullptr);
    jmethodID mainMethod = jvm.GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");
    jvm.CallStaticVoidMethod(mainClass, mainMethod, arr);
    
    std::this_thread::sleep_for(std::chrono::seconds(200));
    
    return 0;
}
