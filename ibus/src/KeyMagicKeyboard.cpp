#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include "KeyMagicKeyboard.h"

KeyMagicString toStringToCharRef(KeyMagicString * str) {
	std::basic_stringstream<wchar_t> out;
	for (KeyMagicString::iterator i = str->begin(); i < str->end(); i++) {
		out << "\\u" << std::hex << (int)*i;
	}
	return out.str();
}

bool sortRule (RuleInfo * r1, RuleInfo * r2) {

	int s1 = r1->getSwitchCount();
	int s2 = r2->getSwitchCount();
	if (s1 != s2) {
		return s1 < s2;
	}

	int v1 = r1->getVKCount();
	int v2 = r2->getVKCount();
	if (v1 != v2) {
		return v1 < v2;
	}

	int m1 = r1->getMatchLength();
	int m2 = r2->getMatchLength();
	if (m1 != m2) {
		return m1 < m2;
	}

	return false;
}

bool KeyMagicKeyboard::loadKeyboardFile(const char * szPath) {
	FileHeader fh;
	FILE * hFile;

	BinaryRuleList rules;
	BinaryStringList strings;

	hFile = fopen(szPath, "rb");
	if (!hFile){
		return false;
	}

	fread(&fh, sizeof(FileHeader), 1, hFile);

	if (fh.magicCode[0] != 'K' || fh.magicCode[1] != 'M' || fh.magicCode[2] != 'K' || fh.magicCode[3] != 'L') {
		return false;
	}

	m_layoutOptions = fh.layoutOptions;

	m_strings.clear();
	m_rules.clear();

	for (int i = 0; i < fh.stringCount; i++)
	{
		short sLength;
		fread(&sLength, sizeof(short), 1, hFile);

		short * local_buf = new short[sLength+1];
		local_buf[sLength]='\0';
		fread(local_buf, sLength * sizeof(short), 1, hFile);

		strings.push_back(local_buf);
	}

	for (int i = 0; i < fh.ruleCount; i++)
	{
		BinaryRule Rule;
		short sLength;

		fread(&sLength, sizeof(short), 1, hFile);
		short * ruleBinaryIn = new short[sLength+1];
		ruleBinaryIn[sLength]='\0';
		fread(ruleBinaryIn, sLength * sizeof(short), 1, hFile);

		fread(&sLength, sizeof(short), 1, hFile);
		short * ruleBinaryOut = new short[sLength+1];
		ruleBinaryOut[sLength]='\0';
		fread(ruleBinaryOut, sLength * sizeof(short), 1, hFile);
		
		Rule.strInRule = (short*)ruleBinaryIn;
		Rule.strOutRule = (short*)ruleBinaryOut;

		rules.push_back(Rule);
	}

	//Convert variable to pure string. Originally, variables are binary sequences
	variablesToStrings(&strings, &m_strings);
	binaryRulesToManagedRules(&rules, &m_rules);
	std::sort(m_rules.begin(), m_rules.end(), sortRule);
	std::reverse(m_rules.begin(), m_rules.end());

	for (RuleList::iterator i = m_rules.begin(); i != m_rules.end(); i++) {
		RuleInfo * ri = *i;
		std::cerr << "sw=" << ri->getSwitchCount() << ";vk=" << ri->getVKCount() << ";str=" << ri->getMatchLength() << std::endl;
	}

	fclose(hFile);

	return true;
}

StringList * KeyMagicKeyboard::getStrings() {
	return &m_strings;
}

RuleList * KeyMagicKeyboard::getRules() {
	return &m_rules;
}

LayoutOptions * KeyMagicKeyboard::getLayoutOptions() {
	return &m_layoutOptions;
}

void KeyMagicKeyboard::variablesToStrings(BinaryStringList * variables, StringList * strings) {

	for (BinaryStringList::iterator i = variables->begin(); i != variables->end(); i++) {

		const short * binString = *i;
		KeyMagicString value;

		while(*binString) {
			switch(*binString) {
			case opVARIABLE:
				binString++;
				value += getVariableValue(*binString++, variables);
				break;
			default:
				value += *binString++;
			}
		}
		strings->push_back(value);
	}
}

void KeyMagicKeyboard::binaryRulesToManagedRules(BinaryRuleList * binRules, RuleList * rules) {
	for (BinaryRuleList::iterator i = binRules->begin(); i != binRules->end(); i++) {
		RuleInfo * managedMatchRule = new RuleInfo(i->strInRule, i->strOutRule, &m_strings);
		managedMatchRule->setIndex(i - binRules->begin());
		rules->push_back(managedMatchRule);
	}
}

bool KeyMagicKeyboard::convertBinaryRuleToManagedRule(const BinaryRule * binRule, RuleInfo * managedMatchRule) {

	return false;
}

KeyMagicString KeyMagicKeyboard::getVariableValue(int index, BinaryStringList * binStrings) {
	KeyMagicString value;

	index--;

	if (index > binStrings->size()) {
		return KeyMagicString();
	}

	const short * binRule = binStrings->at(index);

	while (*binRule) {
		if (*binRule == opVARIABLE) {
			binRule++;
			KeyMagicString value2 = getVariableValue(*binRule++, binStrings);
			value += value2;
		} else {
			value += *binRule++;
		}
	}

	return value;
}
