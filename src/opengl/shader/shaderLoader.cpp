#include "saiga/opengl/shader/shaderLoader.h"


Shader* ShaderLoader::loadFromFile(const std::string &name, const ShaderPart::ShaderCodeInjections &params){
	(void )params; 
	cout << "fail ShaderLoader::loadFromFile "<<name << endl;
	
    SAIGA_ASSERT(0);

    return nullptr;
}

void ShaderLoader::reload(){
    cout<<"ShaderLoader::reload"<<endl;
    for(auto &object : objects){
        auto name = std::get<0>(object);
        auto sci = std::get<1>(object);
        auto shader = std::get<2>(object);


        std::string fullName = shaderPathes.getFile(name);
        auto ret = reload(shader,fullName,sci);
        SAIGA_ASSERT(ret);
    }


}

bool ShaderLoader::reload(Shader *shader, const std::string &name, const ShaderPart::ShaderCodeInjections &sci)
{
    ShaderPartLoader spl(name,sci);
    if(spl.load()){
       spl.reloadShader(shader);
       return true;
    }
    return false;
}
