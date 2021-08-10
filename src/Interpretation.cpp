
/*
Establish interpretations for AST nodes:

-  get any required information from oracle 
-  create coordinates for object
-  update ast-coord bijections
-  create corresponding domain object
-  update coord-domain bijection
-  create element-wise inter object
-  update maps: coords-interp, interp-domain
*/

// TODO: These two can be integrated
#include "Coords.h"
#include "Interpretation.h"
#include "Interp.h"
#include "maps/CoordsToInterp.h"
#include "maps/CoordsToDomain.h"
#include "maps/InterpToDomain.h"
#include "maps/ASTToCoords.h"
#include "oracles/Oracle_AskAll.h"    // default oracle
#include "oracles/Oracle_LeanInference.h"
//#include "Space.h"
#include "Checker.h"

//#include <g3log/g3log.hpp>
#include <unordered_map>
#include <memory>
#include <vector>
using namespace interp;


std::vector<string> *choice_buffer;

Interpretation::Interpretation() { 
    domain_ = new domain::Domain();
    //REPLACE WITH A MULTI-ORACLE
    oracle_ = new oracle::Oracle_AskAll(domain_); 
    oracle_infer_ = new oracle::Oracle_LeanInference(domain_);
    //choice_buffer = new std::vector<string>();
    //oracle_->choice_buffer = choice_buffer;
    /* 
    context_ can only be set later, once Clang starts parse
    */
    ast2coords_ = new ast2coords::ASTToCoords(); 
    coords2dom_ = new coords2domain::CoordsToDomain();
    coords2interp_ = new coords2interp::CoordsToInterp();
    interp2domain_ = new interp2domain::InterpToDomain();
    checker_ = new Checker(this);
}

std::string Interpretation::toStringAST(){
    //this should technically be in Interp but OKAY this second
    //4/13 - nope move this 
    std::string math = "";

    auto astInterp = coords2interp_->getInterp(this->AST);

    auto global_timeseries = domain_->getTimeSeries();

    for(auto coords_ : this->captureCache){
        auto dom_ = this->coords2dom_->getDomain(coords_);
        if(auto dc = dynamic_cast<domain::TimeSeries*>(dom_->getValue())){
            global_timeseries.push_back(dc);
        }
    }

    if(astInterp){
        math+= astInterp->toStringAST(domain_->getSpaces(), global_timeseries);
    }
    else
        std::cout<<"Warning : No top-level AST node present";
    return math;
};

/*
Simple implementation for all nodes - configuration can handle how to differentiate between add and mul nodes, etc.
*/
int global_index = 0; //auto increment for each AST Coords
coords::Coords* Interpretation::mkNode(std::string nodeType, std::shared_ptr<ast::NodeContainer> astNode, bool capture, bool isAST){
    std::vector<coords::Coords*> operand_coords;
    std::vector<domain::DomainContainer*> operand_domains;
    std::vector<interp::Interp*> operand_interps;
    std::vector<coords::Coords*> body_coords;
    std::vector<interp::Interp*> body_interps;


    for(auto child:this->astOperandBuffer){
        operand_coords.push_back(this->ast2coords_->getCoords(child));
    }
    for(auto operand_coord : operand_coords){
        operand_domains.push_back(this->coords2dom_->getDomain(operand_coord));
        operand_interps.push_back(this->coords2interp_->getInterp(operand_coord));
    }

    for(auto child:this->astBodyBuffer){
        body_coords.push_back(this->ast2coords_->getCoords(child));
    }

    for(auto body_coord : body_coords){
        body_interps.push_back(this->coords2interp_->getInterp(body_coord));
    }

    coords::Coords* coords_ = new coords::Coords(nodeType, operand_coords, body_coords);
    coords_->setIndex(global_index++);
    this->ast2coords_->put(astNode, coords_);
    this->ast2coords_->setASTState(coords_,astNode,context_);
    domain::DomainContainer* domain__ = this->domain_->mkDefaultDomainContainer(operand_domains);
    interp::Interp* interp_ = new interp::Interp(coords_, domain__, operand_interps, body_interps);

    this->coords2dom_->put(coords_, domain__);
    this->coords2interp_->put(coords_,interp_);
    this->interp2domain_->put(interp_,domain__);

    if(this->constructor){
        auto cons_coords_ = this->ast2coords_->getCoords(this->constructor);
        auto cons_interp_ = this->coords2interp_->getInterp(cons_coords_);
        interp_->setConstructor(cons_interp_);
    }

    if(this->container){
        auto cont_coords_ = this->ast2coords_->getCoords(this->container);
        cont_coords_->addProperty(coords_);
        coords_->setContainer(cont_coords_);
        auto cont_interp_ = this->coords2interp_->getInterp(cont_coords_);
        cont_interp_->addProperty(interp_);
        interp_->setContainer(cont_interp_);
    }

    if(this->link){
        auto link_coords = this->ast2coords_->getCoords(this->link);
        link_coords->addLink(coords_);
        coords_->setLinked(link_coords);
        auto link_interp = this->coords2interp_->getInterp(link_coords);
        link_interp->addLink(interp_);
        interp_->setLinked(link_interp);
        
    }
    else if(capture){
        if(nodeType.find("BOOL")==string::npos)//this doesn't go here. but just to show about bool uncertainty
            this->captureCache.push_back(coords_);
        else if(nodeType.find("IDENT_BOOL") != string::npos)
            this->captureCache.push_back(coords_);
    }
    if(isAST){
        this->AST = coords_;
    }
    this->clear_buffer();
    this->allCoords.push_back(coords_);

    return coords_;
}

void Interpretation::mkConstructor(std::shared_ptr<ast::NodeContainer> astNode){

    auto coords_ = mkNode("CONSTRUCTOR", astNode, false, false);

    this->constructors.push_back(coords_);
};

void Interpretation::mkFunction(std::shared_ptr<ast::NodeContainer> astNode){

    auto coords_ = mkNode("FUNCTION", astNode, false, false);

    this->functions.push_back(coords_);
};

void Interpretation::mkFunctionWithReturn(std::string nodeRef, std::shared_ptr<ast::NodeContainer> astNode){

    auto coords_ = mkNode("FUNCTION_"+nodeRef, astNode, false, false);

    this->functions_with_return.push_back(coords_);
};

bool Interpretation::tryMkCallExpr(std::shared_ptr<ast::NodeContainer> astNode){
    auto func = this->link;
    auto link_coords = this->ast2coords_->getCoords(this->link);

    if(link_coords){
        std::cout<<"found function!!!";
        this->mkNode(std::string("CALL_")+link_coords->getNodeType(),astNode);
        return true;
    }
    else 
        return false;
};


void Interpretation::mkFunctionCall(std::shared_ptr<ast::NodeContainer> astNode, bool capture){
    auto func = this->link;
    auto link_coords = this->ast2coords_->getCoords(this->link);

    this->mkNode(std::string("CALL_")+link_coords->getNodeType(),astNode);

};

bool Interpretation::checkFuncExists(std::shared_ptr<ast::NodeContainer> astNode){
    auto link_coords = this->ast2coords_->getCoords(astNode);


    return link_coords;
};


void Interpretation::printChoices(){
    aFile* f = new aFile;
    std::string name = "/peirce/annotations.txt";
    char * name_cstr = new char [name.length()+1];
    strcpy (name_cstr, name.c_str());
    f->name = name_cstr;
    f->file = fopen(f->name, "w");
    for(auto choice: this->oracle_->getChoices()){
        fputs((choice + "\n").c_str(), f->file);
    }

    fclose(f->file);
    delete f->name;
    delete f;
};

int consOptionSize = 1;
void Interpretation::printConstructorTable()
{
    int i = consOptionSize+1;//move to "menu offset" global variable
    
    for(auto cons_ : this->constructors)
    {
        auto dom_ = this->coords2dom_->getDomain(cons_);
        std::cout<<"Index: "<<i++<<", Type : Constructor Declaration, Annotation State : "<<dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<cons_->state_->code_
            <<"\n\tExisting Interpretation: "<<dom_->toString()<<std::endl<<std::endl;
        int j = 0;
        for(auto parm_ : cons_->getOperands()){
            auto parm_dom_ = this->coords2dom_->getDomain(parm_);
            std::cout<<"Index: "<<i++<<",Type : Parameter Declaration "<<std::to_string(++j)<<", Annotation State : "<<parm_dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<parm_->state_->code_
                <<"\n\tExisting Interpretation: "<<parm_dom_->toString()<<std::endl<<std::endl;
        }
    }
}

void Interpretation::interpretConstructors(){
    bool needs_infer = false;
    while(true){
        std::string menu = std::string("Options:\n")
                +"0 - Print Constructor Table\n"
                +"1 - Return to Main Menu\n";

                
        int menuSize = consOptionSize;
        std::vector<coords::Coords*> constructorCache;
        for(auto cons_ : this->constructors){
            constructorCache.push_back(cons_);
            menuSize++;
            for(auto parm_ : cons_->getOperands()){
                constructorCache.push_back(parm_);
                menuSize++;
            }
        }
        if(this->constructors.size()>0){
            menu = menu+(std::to_string(consOptionSize+1))+"-"+std::to_string(menuSize)+" - Annotate Node\n";
        }
    
        
        if(needs_infer){
            this->clearInferredInterpretations();
            checker_->RebuildOutput();
            this->performInference();
            //checker_->RebuildOutput(oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));

            this->performInference();
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            needs_infer = false;
        }
        else 
            checker_->RebuildOutput();

        int choice = oracle_->getValidChoice(0, menuSize+1, menu);
        switch(choice)
        {
            case 0:{
                printConstructorTable();
            } break;
            case 1:{
                return;
            } break;
            default:{
                needs_infer = true;
                auto coords_ = constructorCache[choice-consOptionSize-1];
                domain::DomainContainer* dom_cont = this->coords2dom_->getDomain(coords_);
                domain::DomainObject* new_dom = nullptr; 
                if(coords_->getNodeType().find("BOOL")==string::npos)
                    new_dom = this->oracle_->getInterpretation(coords_);
                else
                    new_dom = this->oracle_->getBooleanInterpretation();
                
                if(new_dom){

                    dom_cont->setValue(new_dom);
                    dom_cont->setAnnotationState(domain::AnnotationState::Manual);
                    for(auto link_ : coords_->getLinks()){
                        domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                        link_cont->setValue(new_dom);
                        link_cont->setAnnotationState(domain::AnnotationState::Inferred);
                    }
                }
            };
        }
    }
}

int funcOptionSize = 1;
void Interpretation::printFunctionTable()
{
    int i = funcOptionSize+1;//move to "menu offset" global variable

    for(auto cons_ : this->functions_with_return)
    {
        auto dom_ = this->coords2dom_->getDomain(cons_);
        std::cout<<"Index: "<<i++<<", Function Declaration : "<<cons_->getName()<<", Annotation State : "<<dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<cons_->state_->code_
            <<"\n\tExisting Interpretation: "<<dom_->toString()<<std::endl<<std::endl;
        int j = 0;
        for(auto parm_ : cons_->getOperands()){
            auto parm_dom_ = this->coords2dom_->getDomain(parm_);
            std::cout<<"Index: "<<i++<<", Parameter Declaration "<<std::to_string(++j)<<", Annotation State : "<<parm_dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<parm_->state_->code_
                <<"\n\tExisting Interpretation: "<<parm_dom_->toString()<<std::endl<<std::endl;
        }
    }

    for(auto cons_ : this->functions)
    {
        //auto dom_ = this->coords2dom_->getDomain(cons_);
        int j = 0;
        for(auto parm_ : cons_->getOperands()){
            auto parm_dom_ = this->coords2dom_->getDomain(parm_);
            std::cout<<"Index: "<<i++<<", Parameter Declaration For Function: "<<cons_->getName()<<std::to_string(++j)<<", Annotation State : "<<parm_dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<parm_->state_->code_
                <<"\n\tExisting Interpretation: "<<parm_dom_->toString()<<std::endl<<std::endl;
        }
    }
}

void Interpretation::interpretFunctions(){
    bool needs_infer = false;
    while(true){
        std::string menu = std::string("Options:\n")
                +"0 - Print Function Table\n"
                +"1 - Return to Main Menu\n";

                
        int menuSize = funcOptionSize;
        std::vector<coords::Coords*> functionCache;
        for(auto func_ : this->functions_with_return){
            functionCache.push_back(func_);
            menuSize++;
            for(auto func_ : func_->getOperands()){
                functionCache.push_back(func_);
                menuSize++;
            }
        }
        for(auto func_ : this->functions){
            for(auto parm_ : func_->getOperands()){
                functionCache.push_back(parm_);
                menuSize++;
            }
        }
        if(this->functions.size()+this->functions_with_return.size()>0){
            menu = menu+(std::to_string(funcOptionSize+1))+"-"+std::to_string(menuSize)+" - Annotate Node\n";
        }
    
        
        if(needs_infer){
            this->clearInferredInterpretations();
            checker_->RebuildOutput();
            this->performInference();
            //checker_->RebuildOutput(oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));

            this->performInference();
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            needs_infer = false;
        }
        else 
            checker_->RebuildOutput();

        int choice = oracle_->getValidChoice(0, menuSize+1, menu);
        switch(choice)
        {
            case 0:{
                printFunctionTable();
            } break;
            case 1:{
                return;
            } break;
            default:{
                needs_infer = true;
                
                auto coords_ = functionCache[choice-funcOptionSize-1];
                domain::DomainContainer* dom_cont = this->coords2dom_->getDomain(coords_);
                auto new_dom = this->oracle_->getInterpretation(coords_);
                
                if(new_dom){

                    dom_cont->setValue(new_dom);
                    dom_cont->setAnnotationState(domain::AnnotationState::Manual);
                    for(auto link_ : coords_->getLinks()){
                        domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                        link_cont->setValue(new_dom);
                        link_cont->setAnnotationState(domain::AnnotationState::Inferred);
                    }
                }
            };
        }
    }
}

void Interpretation::clearInferredInterpretations(){
    for(auto coords_ : this->allCoords){
        auto dom_cont = this->coords2dom_->getDomain(coords_);
        if(dom_cont->getAnnotationState() == domain::AnnotationState::Inferred){
            dom_cont->removeInterpretation();
        }
    }
};

void Interpretation::performInference(){
    int totalInferred = 0;
    oracle_infer_->buildInterpretations("PeirceOutput");//move to configuration or method
    for(auto coords_ : this->allCoords){
        /*
        What is the update logic? Very difficult question to answer.
        */
        auto dom_cont = this->coords2dom_->getDomain(coords_);
        auto infer_dom = oracle_infer_->getAllInterpretation(coords_);

        switch(dom_cont->getAnnotationState()){
            case domain::AnnotationState::ManualError :
            case domain::AnnotationState::Manual : {
                //dont overwrite manual annotations
                if(infer_dom){
                    //std::cout<<infer_dom->toString()<<"\n";
                    if(auto dc = dynamic_cast<domain::ErrorObject*>(infer_dom)){
                        dom_cont->setAnnotationState(domain::AnnotationState::ManualError);
                        dom_cont->setError(dc);
                        for(auto link_ : coords_->getLinks()){
                            domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                            link_cont->setError(dc);
                            link_cont->setAnnotationState(domain::AnnotationState::ManualError);
                            
                        }
                    }
                    else{
                        dom_cont->setAnnotationState(domain::AnnotationState::Manual);
                        dom_cont->removeError();
                        
                        for(auto link_ : coords_->getLinks()){
                            domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                            link_cont->removeError();
                            link_cont->setAnnotationState(domain::AnnotationState::Manual);
                            
                        }
                    }

                }
            } break;
            default : {
                if(infer_dom){

                    if(auto dc = dynamic_cast<domain::ErrorObject*>(infer_dom)){
                        dom_cont->setAnnotationState(domain::AnnotationState::Error);
                        dom_cont->setError(dc);
                        
                        for(auto link_ : coords_->getLinks()){
                            domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                            link_cont->setError(dc);
                            link_cont->setAnnotationState(domain::AnnotationState::Error);
                            
                        }
                    }
                    else {
                        dom_cont->removeError();
                        dom_cont->setValue(infer_dom);

                        totalInferred++;
                        
                        for(auto link_ : coords_->getLinks()){
                            domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                            link_cont->removeError();
                            link_cont->setValue(infer_dom);
                            link_cont->setAnnotationState(domain::AnnotationState::Inferred);
                            //totalInferred++;
                        }
                        dom_cont->setAnnotationState(domain::AnnotationState::Inferred);
                    }
                  //  std::cout<<"Index: "<<", Node Type : "<<coords_->getNodeType()<<", Annotation State : "<<dom_cont->getAnnotationStateStr()<<",\n\tSnippet: "<<coords_->state_->code_<<", \n\t"<<coords_->getSourceLoc()
                   // <<"\n\tExisting Interpretation: "<<dom_cont->toString()<<"\n\tError Message: "<<""<<std::endl<<std::endl;
                
    
                }
                else{
                    dom_cont->removeError();
                    dom_cont->removeInterpretation();
                    dom_cont->setAnnotationState(domain::AnnotationState::Unannotated);
                    for(auto link_ : coords_->getLinks()){
                        domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                        link_cont->removeError();
                        link_cont->removeInterpretation();
                        link_cont->setAnnotationState(domain::AnnotationState::Unannotated);
                        //totalInferred++;
                    }
                }
            }
        }
    }
    std::cout<<"Total Inferred Interpretations : "<<std::to_string(totalInferred) + "\n";
}


int optionSize = 8;
/*void Interpretation::printErrors(){
    int i = optionSize+1;//move to "menu offset" global variable
    for(auto coords_ : this->captureCache)
    {
        auto dom_ = this->coords2dom_->getDomain(coords_);

        std::string error_str_ = "No Error Detected";
        //if(dom_->hasValue()){
            if(dom_->hasError()){
                error_str_ = dom_->getError()->toErrorString();
            }
        //}

        std::cout<<"Index: "<<i++<<",Node Type : "<<coords_->getNodeType()<<",\n\tSnippet: "<<coords_->state_->code_<<", \n\t"<<coords_->getSourceLoc()
            <<"\n\tError Message: "<<error_str_<<std::endl;
    }
};
*/
void Interpretation::printAllTerms()
{
    int i = optionSize+1;//move to "menu offset" global variable
    for(auto coords_ : this->allCoords)
    {
        auto nt = coords_->getNodeType();
        //ugly shortcut for now
        if(nt.find("COMPOUND") != string::npos 
            or nt.find("FUNC") != string::npos 
            or nt.find("DECL") != string::npos
            or nt.find("CONSTRUCTOR") != string::npos)
            continue;

        auto dom_ = this->coords2dom_->getDomain(coords_);

        std::string error_str_ = "No Error Detected";
        if(dom_->hasError()){
            error_str_ = dom_->getError()->toErrorString();
        }
        auto code = coords_->state_->code_;
        code = code.size() > 100 ? code.substr(0,100):code;

        std::cout<<"Index: "<<i++<<", Node Type : "<<coords_->getNodeType()<<", Annotation State : "<<dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<coords_->state_->code_<<", \n\t"<<coords_->getSourceLoc()
            <<"\n\tExisting Interpretation: "<<dom_->toString()<<"\n\tError Message: "<<error_str_<<std::endl<<std::endl;
    }
};


void Interpretation::printVarTable(){
    int i = optionSize+1;//move to "menu offset" global variable
    for(auto coords_ : this->captureCache)
    {
        std::string error_str_ = "No Error Detected";

        auto dom_ = this->coords2dom_->getDomain(coords_);
        //if(dom_->hasValue()){
        if(dom_->hasError()){
            error_str_ = dom_->getError()->toErrorString();
        }

        std::cout<<"Index: "<<i++<<", Node Type : "<<coords_->getNodeType()<<", Annotation State : "<<dom_->getAnnotationStateStr()<<",\n\tSnippet: "<<coords_->state_->code_<<", \n\t"<<coords_->getSourceLoc()
            <<"\n\tExisting Interpretation: "<<dom_->toString()<<"\n\tError Message: "<<error_str_<<std::endl<<std::endl;
    }
};
void Interpretation::interpretProgram(){
    bool continue_ = true;
    std::vector<interp::Interp*> ordered_nodes;
    for(auto coords_ : this->captureCache) 
        ordered_nodes.push_back(this->coords2interp_->getInterp(coords_));

    std::vector<interp::Interp*> all_nodes;
    for(auto coords_ : this->allCoords) 
        all_nodes.push_back(this->coords2interp_->getInterp(coords_));


    oracle_infer_->setNodes(ordered_nodes);
    oracle_infer_->setAllNodes(all_nodes);
    bool needs_infer = true;
    while(continue_)
    {
        if(needs_infer){
            this->clearInferredInterpretations();
            checker_->RebuildOutput();
            this->performInference();
            //checker_->RebuildOutput(oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));

            this->performInference();
            checker_->RebuildOutput();//oracle_infer_->leanInferenceOutputStr("PeirceOutput"));
            needs_infer = false;
        }
        else 
            checker_->RebuildOutput();
        this->printChoices();
        std::cout << "********************************************\n";
        std::cout << "See type-checking output in "<<"/peirce/PeirceOutput.lean"<<"\n";
        std::cout << "Annotations stored in " <<"/peirce/annotations.txt"<<"\n";
        std::cout << "********************************************\n";

        int menuSize = optionSize+this->captureCache.size();
        std::string menu = std::string("Options:\n")
            +"0 - Print Table of Annotatable Terms\n"
            +"1 - Print Available Coordinate Spaces\n"
            +"2 - Create Coordinate Space\n"
            +"3 - Create Global Time Series\n"
            +"4 - Add Time Stamped Value to Time Series\n"
            +"5 - Print Table of All Terms\n"
            +"6 - Annotate Constructors\n"
            +"7 - Annotate Functions\n"
            +"8 - Exit and Finish Type Checking\n";
        if(this->captureCache.size()>0){
            menu = menu+(std::to_string(optionSize+1))+"-"+std::to_string(menuSize)+" - Annotate Node\n";
        }
        int choice = oracle_->getValidChoice(0, menuSize+1, menu);
        switch(choice)
        {
            case 0:{
                printVarTable();
            } break;
            case 1:{
                auto spaces = domain_->getSpaces();
                for(auto sp : spaces)
                    std::cout<<sp->toString()<<"\n"; 
            } break;
            case 2:{
                oracle_->getSpace();
            } break;
            case 3:{
                oracle_->buildTimeSeries(nullptr);
            } break;
            case 4:{
                oracle_->addTimeStampedToTimeSeries();
            } break;
            case 5: {
                this->printAllTerms();
            } break;
            case 6: {
                this->interpretConstructors();
                //needs_infer = true;
            } break;
            case 7: {
                this->interpretFunctions();
                //needs_infer = true;
            } break;
            case 8: {
                continue_ = false;
            } break;
            default:{
                needs_infer = true;
                auto coords_ = this->captureCache[choice-optionSize-1];
                domain::DomainContainer* dom_cont = this->coords2dom_->getDomain(coords_);
                domain::DomainObject* new_dom = nullptr; 
                if(coords_->getNodeType().find("BOOL")==string::npos)
                    new_dom = this->oracle_->getInterpretation(coords_);
                else
                    new_dom = this->oracle_->getBooleanInterpretation();
                
                if(new_dom){

                    dom_cont->setValue(new_dom);
                    dom_cont->setAnnotationState(domain::AnnotationState::Manual);
                    for(auto link_ : coords_->getLinks()){
                        domain::DomainContainer* link_cont = this->coords2dom_->getDomain(link_);
                        link_cont->setValue(new_dom);
                        link_cont->setAnnotationState(domain::AnnotationState::Inferred);
                    }
                }
            };
        }
    }
};
