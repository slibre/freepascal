{$mode objfpc}
{$h+}
{
    This file is part of the Free Component Library (FCL)
    Copyright (c) 2004 by Dean Zobec, Michael Van Canneyt

    an example of a console test runner of FPCUnit tests.

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************}
program frameworktest;

uses
  custapp, classes, SysUtils, fpcunit, testreport, asserttest, suitetest;

Const
  ShortOpts = 'alh';
  Longopts : Array[1..5] of String = (
    'all','list','format:','suite:','help');
  Version = 'Version 0.1';

Type
  TTestRunner = Class(TCustomApplication)
  private
    FSuite: TTestSuite;
    FXMLResultsWriter: TXMLResultsWriter;
  protected
    procedure DoRun ; Override;
    procedure doTestRun(aTest: TTest); virtual;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
  end;


constructor TTestRunner.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FXMLResultsWriter := TXMLResultsWriter.Create;
  FSuite := TTestSuite.Create;
  FSuite.TestName := 'Framework test';
  FSuite.AddTestSuiteFromClass(TAssertTest);
  FSuite.AddTestSuiteFromClass(TTestIgnore);
  FSuite.AddTest(TSuiteTest.Suite());
end;

destructor TTestRunner.Destroy;
begin
  FXMLResultsWriter.Free;
  FSuite.Free;
end;

procedure TTestRunner.doTestRun(aTest: TTest);
var
  testResult: TTestResult;
begin
  testResult := TTestResult.Create;
  try
    testResult.AddListener(FXMLResultsWriter);
    FXMLResultsWriter.WriteHeader;
    aTest.Run(testResult);
    FXMLResultsWriter.WriteResult(testResult);
  finally
    testResult.Free;
  end;
end;

procedure TTestRunner.DoRun;
var
  I : Integer;
  S : String;
begin
  S:=CheckOptions(ShortOpts,LongOpts);
  If (S<>'') then
    Writeln(S);
  if HasOption('h', 'help') or (ParamCount = 0) then
  begin
    writeln(Title);
    writeln(Version);
    writeln('Usage: ');
    writeln('-l or --list to show a list of registered tests');
    writeln('default format is xml, add --format=latex to output the list as latex source');
    writeln('-a or --all to run all the tests and show the results in xml format');
    writeln('The results can be redirected to an xml file,');
    writeln('for example: ./testrunner --all > results.xml');
    writeln('use --suite=MyTestSuiteName to run only the tests in a single test suite class');
  end
  else;
    if HasOption('l', 'list') then
    begin
      if HasOption('format') then
      begin
        if GetOptionValue('format') = 'latex' then
          writeln(GetSuiteAsLatex(FSuite))
        else
          writeln(GetSuiteAsXML(FSuite));
      end
      else
        writeln(GetSuiteAsXML(FSuite));
    end;
  if HasOption('a', 'all') then
  begin
    doTestRun(FSuite)
  end
  else
    if HasOption('suite') then
    begin
      S := '';
      S:=GetOptionValue('suite');
      if S = '' then
        for I := 0 to FSuite.Tests.count - 1 do
          writeln(FSuite[i].TestName)
      else
      for I := 0 to FSuite.Tests.count - 1 do
        if FSuite[i].TestName = S then
        begin
          doTestRun(FSuite.Test[i]);
        end;
    end;
  Terminate;
end;

Var
  App : TTestRunner;

begin
  App:=TTestRunner.Create(Nil);
  App.Initialize;
  App.Title := 'FPCUnit Console Test Case runner.';
  App.Run;
  App.Free;
end.
