#pragma once

#include "Parsing/Simple/SimpleParserValue.h"
#include "Parsing/Matcher/AbstractMatcher.h"

class MenuMatcherScriptInt final : public AbstractMatcher<SimpleParserValue>
{
protected:
    MatcherResult<SimpleParserValue> CanMatch(ILexer<SimpleParserValue>* lexer, unsigned tokenOffset) override;

public:
    MenuMatcherScriptInt();
};
