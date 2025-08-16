/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2025 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_XMLDOCUMENT_H_INCLUDED
#define WATER_XMLDOCUMENT_H_INCLUDED

#include "../text/StringArray.h"

#include "extra/ScopedPointer.hpp"

namespace water {

//==============================================================================
/**
    Parses a text-based XML document and creates an XmlElement object from it.

    The parser will parse DTDs to load external entities but won't
    check the document for validity against the DTD.

    e.g.
    @code

    XmlDocument myDocument (File ("myfile.xml"));
    ScopedPointer<XmlElement> mainElement (myDocument.getDocumentElement());

    if (mainElement == nullptr)
    {
        String error = myDocument.getLastParseError();
    }
    else
    {
        ..use the element
    }

    @endcode

    Or you can use the static helper methods for quick parsing..

    @code
    ScopedPointer<XmlElement> xml (XmlDocument::parse (myXmlFile));

    if (xml != nullptr && xml->hasTagName ("foobar"))
    {
        ...etc
    @endcode

    @see XmlElement
*/
class XmlDocument
{
public:
    //==============================================================================
    /** Creates an XmlDocument from the xml text.
        The text doesn't actually get parsed until the getDocumentElement() method is called.
    */
    XmlDocument (const String& documentText);

    /** Creates an XmlDocument from a file.
        The text doesn't actually get parsed until the getDocumentElement() method is called.
    */
    XmlDocument (const File& file);

    /** Destructor. */
    ~XmlDocument();

    //==============================================================================
    /** Creates an XmlElement object to represent the main document node.

        This method will do the actual parsing of the text, and if there's a
        parse error, it may returns nullptr (and you can find out the error using
        the getLastParseError() method).

        See also the parse() methods, which provide a shorthand way to quickly
        parse a file or string.

        @param onlyReadOuterDocumentElement     if true, the parser will only read the
                                                first section of the file, and will only
                                                return the outer document element - this
                                                allows quick checking of large files to
                                                see if they contain the correct type of
                                                tag, without having to parse the entire file
        @returns    a new XmlElement which the caller will need to delete, or null if
                    there was an error.
        @see getLastParseError
    */
    XmlElement* getDocumentElement (bool onlyReadOuterDocumentElement = false);

    /** Returns the parsing error that occurred the last time getDocumentElement was called.

        @returns the error, or an empty string if there was no error.
    */
    const String& getLastParseError() const noexcept;

    /** Sets an input source object to use for parsing documents that reference external entities.

        If the document has been created from a file, this probably won't be needed, but
        if you're parsing some text and there might be a DTD that references external
        files, you may need to create a custom input source that can retrieve the
        other files it needs.

        The object that is passed-in will be deleted automatically when no longer needed.

        @see FileInputSource
    */
    void setInputSource (FileInputSource* newSource) noexcept;

    /** Sets a flag to change the treatment of empty text elements.

        If this is true (the default state), then any text elements that contain only
        whitespace characters will be ingored during parsing. If you need to catch
        whitespace-only text, then you should set this to false before calling the
        getDocumentElement() method.
    */
    void setEmptyTextElementsIgnored (bool shouldBeIgnored) noexcept;

    //==============================================================================
    /** A handy static method that parses a file.
        This is a shortcut for creating an XmlDocument object and calling getDocumentElement() on it.
        @returns    a new XmlElement which the caller will need to delete, or null if there was an error.
    */
    static XmlElement* parse (const File& file);

    /** A handy static method that parses some XML data.
        This is a shortcut for creating an XmlDocument object and calling getDocumentElement() on it.
        @returns    a new XmlElement which the caller will need to delete, or null if there was an error.
    */
    static XmlElement* parse (const String& xmlData);


    //==============================================================================
private:
    String originalText;
    CharPointer_UTF8 input;
    bool outOfData, errorOccurred;

    String lastError, dtdText;
    StringArray tokenisedDTD;
    bool needToLoadDTD, ignoreEmptyTextElements;
    DISTRHO_NAMESPACE::ScopedPointer<FileInputSource> inputSource;

    XmlElement* parseDocumentElement (CharPointer_UTF8, bool outer);
    void setLastError (const String&, bool carryOn);
    bool parseHeader();
    bool parseDTD();
    void skipNextWhiteSpace();
    water_uchar readNextChar() noexcept;
    XmlElement* readNextElement (bool alsoParseSubElements);
    void readChildElements (XmlElement&);
    void readQuotedString (String&);
    void readEntity (String&);

    String getFileContents (const String&) const;
    String expandEntity (const String&);
    String expandExternalEntity (const String&);
    String getParameterEntity (const String&);

    CARLA_DECLARE_NON_COPYABLE (XmlDocument)
};

}

#endif // WATER_XMLDOCUMENT_H_INCLUDED
