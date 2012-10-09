/**
 * \file Info.h
 * \brief Declaration of the Info class
 * \author Rahul Nanda, Julien Henry
 */
#ifndef _INFO_H
#define _INFO_H

#include<string>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/CFG.h"

extern llvm::raw_ostream *Out;

/**
 * \class Info
 * \brief Info class holds information about a variable in original source-code and provides necessary functions.
 */
class Info
{
	std::string name;
	int lineNo;
	std::string type;

	public:
	Info();
	Info(std::string name,int lineNo,std::string type)
	{
		this->name=name;
		this->lineNo=lineNo;
		this->type=type;
	}
	void display()
	{
		*Out<<"("<<type<<" "<<name<<" "<<lineNo<<")";
	}
	std::string getName(){
		return name;}
	std::string getType(){
		return type;}
	int getLineNo(){
		return lineNo;}
	bool isEqual(Info* I) //true if equal
	{
		if(!name.compare(I->getName()) && !type.compare(I->getType()) && lineNo==I->getLineNo())
			return true;
		return false;
	}
};

#endif
