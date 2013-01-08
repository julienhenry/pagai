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
	Info() {
		lineNo=-1;
	}

	Info(std::string _name,int _lineNo,std::string _type) {
		name=_name;
		lineNo=_lineNo;
		type=_type;
	}

	Info(const Info& I): name(I.name), lineNo(I.lineNo), type(I.type) {}

	Info& operator=(const Info &I) {
		name = I.name;
		lineNo = I.lineNo;
		type = I.type;
		return *this;
	}

	void display() const {
		*Out<<"(type="<<type<<" name="<<name<<" line="<<lineNo<<")";
	}

	std::string getName() {return name;}

	std::string getType() {return type;}

	int getLineNo() {return lineNo;}

	bool empty() {return name.empty();}

	int Compare (const Info& i) const {
		int name_comparison = name.compare(i.name);
		int type_comparison = type.compare(i.type);
		if(type_comparison == 0 && name_comparison == 0 && lineNo==i.lineNo) return 0;
		if(lineNo < i.lineNo) {
			return -1;
		} else if (lineNo > i.lineNo) {
			return 1;
		} else {
			if (name_comparison != 0) return name_comparison;
			return type_comparison;
		}
	}

	bool operator == (const Info& i) const {
		return !Compare(i);
	}

	bool operator < (const Info& i) const {
		return Compare(i)<0;   
	}
};

#endif
