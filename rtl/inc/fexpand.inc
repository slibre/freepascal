{
    This file is part of the Free Pascal run time library.
    Copyright (c) 1997-2000 by the Free Pascal development team

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************}

{****************************************************************************
                A platform independent FExpand implementation
****************************************************************************}

{$IFDEF FPC_FEXPAND_VOLUMES}
 {$IFNDEF FPC_FEXPAND_DRIVES}
  (* Volumes are just a special case of drives. *)
  {$DEFINE FPC_FEXPAND_DRIVES}
 {$ENDIF FPC_FEXPAND_DRIVES}
{$ENDIF FPC_FEXPAND_VOLUMES}

{$IFDEF FPC_FEXPAND_DIRSEP_IS_CURDIR}
 {$IFNDEF FPC_FEXPAND_DRIVES}
  (* If DirectorySeparator at the beginning marks a relative path, *)
  (* an absolute path must always begin with a drive or volume.    *)
  {$DEFINE FPC_FEXPAND_DRIVES}
 {$ENDIF FPC_FEXPAND_DRIVES}
 {$IFNDEF FPC_FEXPAND_MULTIPLE_UPDIR}
  (* Traversing multiple levels at once explicitely allowed. *)
  {$DEFINE FPC_FEXPAND_MULTIPLE_UPDIR}
 {$ENDIF FPC_FEXPAND_MULTIPLE_UPDIR}
 (* Helper define used to support common features of FPC_FEXPAND_DIRSEP_IS_* *)
 {$DEFINE FPC_FEXPAND_UPDIR_HELPER}
{$ENDIF FPC_FEXPAND_DIRSEP_IS_CURDIR}

{$IFDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
 {$IFNDEF FPC_FEXPAND_DRIVES}
  (* If DirectorySeparator at the beginning marks a relative path, *)
  (* an absolute path must always begin with a drive or volume.    *)
  {$DEFINE FPC_FEXPAND_DRIVES}
 {$ENDIF FPC_FEXPAND_DRIVES}
 {$IFNDEF FPC_FEXPAND_MULTIPLE_UPDIR}
  (* Traversing multiple levels at once explicitely allowed. *)
  {$DEFINE FPC_FEXPAND_MULTIPLE_UPDIR}
 {$ENDIF FPC_FEXPAND_MULTIPLE_UPDIR}
 (* Helper define used to support common features of FPC_FEXPAND_DIRSEP_IS_* *)
 {$DEFINE FPC_FEXPAND_UPDIR_HELPER}
{$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}

procedure GetDirIO (DriveNr: byte; var Dir: String);

(* GetDirIO is supposed to return the root of the given drive   *)
(* in case of an error for compatibility of FExpand with TP/BP. *)

var
  OldInOutRes: word;
begin
  OldInOutRes := InOutRes;
  InOutRes := 0;
  GetDir (DriveNr, Dir);
  InOutRes := OldInOutRes;
end;


{$IFDEF FPC_FEXPAND_VOLUMES}
 {$IFNDEF FPC_FEXPAND_NO_DEFAULT_PATHS}
procedure GetDirIO (const VolumeName: OpenString; var Dir: string);

var
  OldInOutRes: word;
begin
  OldInOutRes := InOutRes;
  InOutRes := 0;
  GetDir (VolumeName, Dir);
  InOutRes := OldInOutRes;
end;
 {$ENDIF FPC_FEXPAND_NO_DEFAULT_PATHS}
{$ENDIF FPC_FEXPAND_VOLUMES}


function FExpand (const Path: PathStr): PathStr;

(* LFNSupport boolean constant, variable or function must be declared for all
   the platforms, at least locally in the Dos unit implementation part.
   In addition, FPC_FEXPAND_UNC, FPC_FEXPAND_DRIVES, FPC_FEXPAND_GETENV_PCHAR,
   FPC_FEXPAND_TILDE, FPC_FEXPAND_VOLUMES, FPC_FEXPAND_NO_DEFAULT_PATHS,
   FPC_FEXPAND_DRIVESEP_IS_ROOT, FPC_FEXPAND_NO_CURDIR,
   FPC_FEXPAND_NO_DOTS_UPDIR, FPC_FEXPAND_DIRSEP_IS_UPDIR,
   FPC_FEXPAND_DIRSEP_IS_CURDIR and FPC_FEXPAND_MULTIPLE_UPDIR conditionals
   might be defined to specify FExpand behaviour - see end of this file for
   individual descriptions. Finally, FPC_FEXPAND_SYSUTILS allows to reuse
   the same implementation for SysUtils.ExpandFileName.
*)

{$IFDEF FPC_FEXPAND_DRIVES}
var
    PathStart: longint;
{$ELSE FPC_FEXPAND_DRIVES}
const
    PathStart = 1;
{$ENDIF FPC_FEXPAND_DRIVES}
{$IFDEF FPC_FEXPAND_UNC}
var
    RootNotNeeded: boolean;
{$ELSE FPC_FEXPAND_UNC}
const
    RootNotNeeded = false;
{$ENDIF FPC_FEXPAND_UNC}

var S, Pa, Dirs: PathStr;
    I, J: longint;

begin
{$IFDEF FPC_FEXPAND_UNC}
    RootNotNeeded := false;
{$ENDIF FPC_FEXPAND_UNC}

(* First convert the path to uppercase if appropriate for current platform. *)
    if FileNameCasePreserving then
        Pa := Path
    else
        Pa := UpCase (Path);

(* Allow both '/' and '\' as directory separators *)
(* by converting all to the native one.           *)
{$warnings off}
	  for I := 1 to Length (Pa) do
	    if Pa [I] in AllowDirectorySeparators  then
	      Pa [I] := DirectorySeparator;
{$warnings on}

(* PathStart is amount of characters to strip to get beginning *)
(* of path without volume/drive specification.                 *)
{$IFDEF FPC_FEXPAND_DRIVES}
 {$IFDEF FPC_FEXPAND_VOLUMES}
  {$IFDEF FPC_FEXPAND_DRIVESEP_IS_ROOT}
    PathStart := Pos (DriveSeparator, Pa);
  {$ELSE FPC_FEXPAND_DRIVESEP_IS_ROOT}
    PathStart := Succ (Pos (DriveSeparator, Pa));
  {$ENDIF FPC_FEXPAND_DRIVESEP_IS_ROOT}
 {$ELSE FPC_FEXPAND_VOLUMES}
    PathStart := 3;
 {$ENDIF FPC_FEXPAND_VOLUMES}
{$ENDIF FPC_FEXPAND_DRIVES}

(* Expand tilde to home directory if appropriate. *)
{$IFDEF FPC_FEXPAND_TILDE}
    {Replace ~/ with $HOME/}
    if (Length (Pa) >= 1) and (Pa [1] = '~') and
                      ((Pa [2] = DirectorySeparator) or (Length (Pa) = 1)) then
        begin
 {$IFDEF FPC_FEXPAND_SYSUTILS}
            S := GetEnvironmentVariable ('HOME');
 {$ELSE FPC_FEXPAND_SYSUTILS}
  {$IFDEF FPC_FEXPAND_GETENV_PCHAR}
            S := StrPas (GetEnv ('HOME'));
  {$ELSE FPC_FEXPAND_GETENV_PCHAR}
            S := GetEnv ('HOME');
  {$ENDIF FPC_FEXPAND_GETENV_PCHAR}
 {$ENDIF FPC_FEXPAND_SYSUTILS}
            if (S = '') or (Length (S) = 1) and (Length (Pa) > 1)
                                          and (S [1] = DirectorySeparator) then
                Delete (Pa, 1, 1)
            else
                if S [Length (S)] = DirectorySeparator then
                    Pa := S + Copy (Pa, 3, Length (Pa) - 2)
                else
                    Pa := S + Copy (Pa, 2, Pred (Length (Pa)));
        end;
{$ENDIF FPC_FEXPAND_TILDE}

(* Do we have a drive/volume specification? *)
{$IFDEF FPC_FEXPAND_VOLUMES}
    if PathStart > 1 then
{$ELSE FPC_FEXPAND_VOLUMES}
    if (Length (Pa) > 1) and (Pa [1] in ['A'..'Z', 'a'..'z']) and
      (Pa [2] = DriveSeparator) and (DriveSeparator <> DirectorySeparator) then
{$ENDIF FPC_FEXPAND_VOLUMES}
        begin

(* We need to know current directory on given *)
(* volume/drive _if_ such a thing is defined. *)
{$IFDEF FPC_FEXPAND_DRIVES}
 {$IFNDEF FPC_FEXPAND_NO_DEFAULT_PATHS}
  {$IFDEF FPC_FEXPAND_VOLUMES}
            GetDirIO (Copy (Pa, 1, PathStart - 2), S);
  {$ELSE FPC_FEXPAND_VOLUMES}
            { Always uppercase driveletter }
            if (Pa [1] in ['a'..'z']) then
                Pa [1] := Chr (Ord (Pa [1]) and not ($20));
            GetDirIO (Ord (Pa [1]) - Ord ('A') + 1, S);
  {$ENDIF FPC_FEXPAND_VOLUMES}

(* Do we have more than just drive/volume specification? *)
            if Length (Pa) = Pred (PathStart) then

(* If not, just use the current directory for that drive/volume. *)
                Pa := S
            else

(* If yes, find out whether the following path is relative or absolute. *)
                if Pa [PathStart] <> DirectorySeparator then
  {$IFDEF FPC_FEXPAND_VOLUMES}
                    if Copy (Pa, 1, PathStart - 2) = Copy (S, 1, PathStart - 2)
                                                                           then
  {$ELSE FPC_FEXPAND_VOLUMES}
                    if UpCase(Pa [1]) = UpCase(S [1]) then
  {$ENDIF FPC_FEXPAND_VOLUMES}
                        begin
                            { remove ending slash if it already exists }
                            if S [Length (S)] = DirectorySeparator then
                               SetLength(S,Length(s)-1);
                            Pa := S + DirectorySeparator +
                              Copy (Pa, PathStart, Length (Pa) - PathStart + 1)
                        end
                    else
  {$IFDEF FPC_FEXPAND_VOLUMES}
                        Pa := Copy (Pa, 1, PathStart - 2) + DriveSeparator
                           + DirectorySeparator +
                              Copy (Pa, PathStart, Length (Pa) - PathStart + 1)
  {$ELSE FPC_FEXPAND_VOLUMES}
                        Pa := Pa [1] + DriveSeparator + DirectorySeparator +
                              Copy (Pa, PathStart, Length (Pa) - PathStart + 1)
  {$ENDIF FPC_FEXPAND_VOLUMES}
 {$ENDIF FPC_FEXPAND_NO_DEFAULT_PATHS}
        end
    else
{$ELSE FPC_FEXPAND_DRIVES}

(* If drives are not supported, but a drive *)
(* was supplied anyway, ignore (remove) it. *)
            Delete (Pa, 1, 2);
        end;
    {Check whether we don't have an absolute path already}
    if (Length (Pa) >= PathStart) and (Pa [PathStart] <> DirectorySeparator) or
                                                 (Length (Pa) < PathStart) then
{$ENDIF FPC_FEXPAND_DRIVES}
        begin

(* Get current directory on selected drive/volume. *)
            GetDirIO (0, S);
{$IFDEF FPC_FEXPAND_VOLUMES}
 {$IFDEF FPC_FEXPAND_DRIVESEP_IS_ROOT}
            PathStart := Pos (DriveSeparator, S);
 {$ELSE FPC_FEXPAND_DRIVESEP_IS_ROOT}
            PathStart := Succ (Pos (DriveSeparator, S));
 {$ENDIF FPC_FEXPAND_DRIVESEP_IS_ROOT}
{$ENDIF FPC_FEXPAND_VOLUMES}

(* Do we have an absolute path without drive or volume? *)
{$IFNDEF FPC_FEXPAND_DIRSEP_IS_CURDIR}
 {$IFDEF FPC_FEXPAND_DRIVES}
            if (Length (Pa) > 0)
  {$IFDEF FPC_FEXPAND_DRIVESEP_IS_ROOT}
                                 and (Pa [1] = DriveSeparator)
  {$ELSE FPC_FEXPAND_DRIVESEP_IS_ROOT}
                                 and (Pa [1] = DirectorySeparator)
  {$ENDIF FPC_FEXPAND_DRIVESEP_IS_ROOT}
                                                                   then
                begin
  {$IFDEF FPC_FEXPAND_UNC}
                    {Do not touch network drive names}
                    if (Length (Pa) > 1) and (Pa [2] = DirectorySeparator)
                                                            and LFNSupport then
                        begin
                            PathStart := 3;
                            {Find the start of the string of directories}
                            while (PathStart <= Length (Pa)) and
                                      (Pa [PathStart] <> DirectorySeparator) do
                                Inc (PathStart);
                            if PathStart > Length (Pa) then
                            {We have just a machine name...}
                                if Length (Pa) = 2 then
                                {...or not even that one}
                                    PathStart := 2
                                else
                                    Pa := Pa + DirectorySeparator                            else
                                if PathStart < Length (Pa) then
                                {We have a resource name as well}
                                    begin
                                        RootNotNeeded := true;
                                        {Let's continue in searching}
                                        repeat
                                            Inc (PathStart);
                                        until (PathStart > Length (Pa)) or
                                         (Pa [PathStart] = DirectorySeparator);
                                    end;
                        end
                    else
  {$ENDIF FPC_FEXPAND_UNC}
  {$IFDEF FPC_FEXPAND_VOLUMES}
                        begin
                            I := Pos (DriveSeparator, S);
   {$IFDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
    {$IFDEF FPC_FEXPAND_DRIVESEP_IS_ROOT}
                            if (Pa [1] = DriveSeparator) then
                                Delete (Pa, 1, 1);
    {$ENDIF FPC_FEXPAND_DRIVESEP_IS_ROOT}
                            Pa := Copy (S, 1, I) + Pa;
                            PathStart := I;
   {$ELSE FPC_FEXPAND_DIRSEP_IS_UPDIR}
                            Pa := Copy (S, 1, Pred (I)) + DriveSeparator + Pa;
                            PathStart := Succ (I);
   {$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}
                        end;
  {$ELSE FPC_FEXPAND_VOLUMES}
                        Pa := S [1] + DriveSeparator + Pa;
  {$ENDIF FPC_FEXPAND_VOLUMES}
                end
            else
 {$ENDIF FPC_FEXPAND_DRIVES}

                (* We already have a slash if root is the curent directory. *)
                if Length (S) = PathStart then
                    Pa := S + Pa
{$ELSE FPC_FEXPAND_DIRSEP_IS_CURDIR}
                (* More complex with DirectorySeparator as current directory *)
                if (S [Length (S)] = DriveSeparator)
                                         and (Pa [1] = DirectorySeparator) then
                    Pa := S + Copy (Pa, 2, Pred (Length (Pa)))
{$ENDIF FPC_FEXPAND_DIRSEP_IS_CURDIR}
                else

                    (* We need an ending slash if FExpand was called  *)
                    (* with an empty string for compatibility, except *)
                    (* for platforms where this is invalid.           *)
                    if Length (Pa) = 0 then
{$IFDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
                        Pa := S
{$ELSE FPC_FEXPAND_DIRSEP_IS_UPDIR}
                        Pa := S + DirectorySeparator
{$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}
                    else
{$IFDEF FPC_FEXPAND_UPDIR_HELPER}
                        if Pa [1] = DirectorySeparator then
                            Pa := S + Pa
                        else
{$ENDIF FPC_FEXPAND_UPDIR_HELPER}
                        Pa := S + DirectorySeparator + Pa;
        end;

    {Get string of directories to only process relative references on this one}
    Dirs := Copy (Pa, Succ (PathStart), Length (Pa) - PathStart);

{$IFNDEF FPC_FEXPAND_NO_CURDIR}
 {$IFNDEF FPC_FEXPAND_DIRSEP_IS_CURDIR}
    {First remove all references to '\.\'}
    I := Pos (DirectorySeparator + '.' + DirectorySeparator, Dirs);
    while I <> 0 do
        begin
            Delete (Dirs, I, 2);
            I := Pos (DirectorySeparator + '.' + DirectorySeparator, Dirs);
        end;
 {$ENDIF FPC_FEXPAND_DIRSEP_IS_CURDIR}
{$ENDIF FPC_FEXPAND_NO_CURDIR}

{$IFNDEF FPC_FEXPAND_NO_DOTS_UPDIR}
 {$IFDEF FPC_FEXPAND_MULTIPLE_UPDIR}
    {Now replace all references to '\...' with '\..\..'}
    I := Pos (DirectorySeparator + '...', Dirs);
    while I <> 0 do
        begin
            Insert (DirectorySeparator + '.', Dirs, I + 3);
            I := Pos (DirectorySeparator + '...', Dirs);
        end;
 {$ENDIF FPC_FEXPAND_MULTIPLE_UPDIR}

    {Now remove also all references to '\..\' + of course previous dirs..}
    I := Pos (DirectorySeparator + '..' + DirectorySeparator, Dirs);
    while I <> 0 do
        begin
            J := Pred (I);
            while (J > 0) and (Dirs [J] <> DirectorySeparator) do
                Dec (J);
            Delete (Dirs, Succ (J), I - J + 3);
            I := Pos (DirectorySeparator + '..' + DirectorySeparator, Dirs);
        end;
{$ENDIF FPC_FEXPAND_NO_DOTS_UPDIR}

{$IFDEF FPC_FEXPAND_UPDIR_HELPER}
    (* Now remove all references to '//' plus previous directories... *)
    I := Pos (DirectorySeparator + DirectorySeparator, Dirs);
    while I <> 0 do
        begin
            J := Pred (I);
            while (J > 0) and (Dirs [J] <> DirectorySeparator) do
                Dec (J);
            Delete (Dirs, Succ (J), Succ (I - J));
            I := Pos (DirectorySeparator + DirectorySeparator, Dirs);
        end;
{$ENDIF FPC_FEXPAND_UPDIR_HELPER}

{$IFNDEF FPC_FEXPAND_NO_DOTS_UPDIR}
    {Then remove also a reference to '\..' at the end of line
    + the previous directory, of course,...}
    I := Pos (DirectorySeparator + '..', Dirs);
    if (I <> 0) and (I = Length (Dirs) - 2) then
        begin
            J := Pred (I);
            while (J > 0) and (Dirs [J] <> DirectorySeparator) do
                Dec (J);
            if (J = 0) then
                Dirs := ''
            else
                Delete (Dirs, Succ (J), I - J + 2);
        end;
{$ENDIF FPC_FEXPAND_NO_DOTS_UPDIR}

{$IFDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
    (* Remove a possible reference to '/' at the *)
    (* end of line plus the previous directory.  *)
    I := Length (Dirs);
    if (I > 0) and (Dirs [I] = DirectorySeparator) then
        begin
            J := Pred (I);
            while (J > 0) and (Dirs [J] <> DirectorySeparator) do
                Dec (J);
            if (J = 0) then
                Dirs := ''
            else
                Delete (Dirs, J, Succ (I - J));
        end;
{$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}

{$IFNDEF FPC_FEXPAND_NO_CURDIR}
 {$IFNDEF FPC_FEXPAND_DIRSEP_IS_CURDIR}
    {...and also a possible reference to '\.'}
    if (Length (Dirs) = 1) then
        begin
            if (Dirs [1] = '.') then
            {A special case}
                Dirs := ''
        end
    else
        if (Length (Dirs) <> 0) and (Dirs [Length (Dirs)] = '.') and
                        (Dirs [Pred (Length (Dirs))] = DirectorySeparator) then
            Delete (Dirs,length(Dirs)-1,2);

    {Finally remove '.\' at the beginning of the string of directories...}
    while (Length (Dirs) >= 2) and (Dirs [1] = '.')
                                         and (Dirs [2] = DirectorySeparator) do
        Delete (Dirs, 1, 2);
 {$ENDIF FPC_FEXPAND_DIRSEP_IS_CURDIR}
{$ENDIF FPC_FEXPAND_NO_CURDIR}

{$IFDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
    (* Remove possible (invalid) references to '/' at the beginning. *)
    while (Length (Dirs) >= 1) and (Dirs [1] = '/') do
        Delete (Dirs, 1, 1);
{$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}

{$IFNDEF FPC_FEXPAND_NO_DOTS_UPDIR}
    {...and possible (invalid) references to '..\' as well}
    while (Length (Dirs) >= 3) and (Dirs [1] = '.') and (Dirs [2] = '.') and
                                             (Dirs [3] = DirectorySeparator) do
        Delete (Dirs, 1, 3);
{$ENDIF FPC_FEXPAND_NO_DOTS_UPDIR}

    {Two special cases - '.' and '..' alone}
{$IFNDEF FPC_FEXPAND_NO_CURDIR}
 {$IFNDEF FPC_FEXPAND_DIRSEP_IS_CURDIR}
    if (Length (Dirs) = 1) and (Dirs [1] = '.') then
        Dirs := '';
 {$ENDIF FPC_FEXPAND_DIRSEP_IS_CURDIR}
{$ENDIF FPC_FEXPAND_NO_CURDIR}
{$IFNDEF FPC_FEXPAND_NO_DOTS_UPDIR}
    if (Length (Dirs) = 2) and (Dirs [1] = '.') and (Dirs [2] = '.') then
        Dirs := '';
{$ENDIF FPC_FEXPAND_NO_DOTS_UPDIR}

    {Join the parts back to create the complete path}
    if Length (Dirs) = 0 then
        begin
            Pa := Copy (Pa, 1, PathStart);
{$IFNDEF FPC_FEXPAND_DRIVESEP_IS_ROOT}
            if Pa [PathStart] <> DirectorySeparator then
                Pa := Pa + DirectorySeparator;
{$ENDIF FPC_FEXPAND_DRIVESEP_IS_ROOT}
        end
    else
        Pa := Copy (Pa, 1, PathStart) + Dirs;

{$IFNDEF FPC_FEXPAND_DIRSEP_IS_UPDIR}
    {Remove ending \ if not supplied originally, the original string
    wasn't empty (to stay compatible) and if not really needed}
    if (Pa [Length (Pa)] = DirectorySeparator)
         and ((Length (Pa) > PathStart) or
{A special case with UNC paths}
            (RootNotNeeded and (Length (Pa) = PathStart)))
    {Reference to current directory at the end should be removed}
                    and (Length (Path) <> 0)
                          and (Path [Length (Path)] <> DirectorySeparator)
                                                                           then
        Delete (PA,length(PA),1);
{$ENDIF FPC_FEXPAND_DIRSEP_IS_UPDIR}

    FExpand := Pa;
end;

(* Description of individual conditional defines supported for FExpand
   (disregard the used directory separators in examples, constant
   System.DirectorySeparator is used in the real implemenation, of course):

   FPC_FEXPAND_UNC - UNC ("Universal Naming Convention") paths are
   supported (usually used for networking, used in DOS (with
   networking support installed), OS/2, Win32 and at least some
   Netware versions as far as I remember. An example of such a path
   is '\\servername\sharename\some\path'.

   FPC_FEXPAND_DRIVES - drive letters are supported (DOS-like
   environments - DOS, OS/2, Win32). Example is 'C:\TEST'.

   FPC_FEXPAND_GETENV_PCHAR - an implementation of GetEnv returning
   PChar instead of a shortstring is available (Unix) to support
   long values of environment variables.

   FPC_FEXPAND_TILDE - expansion of '~/' to GetEnv('HOME') - Unix.
   Example: '~/some/path'.

   FPC_FEXPAND_VOLUMES - volumes are supported (similar to drives,
   but the name can be longer; used under Netware, Amiga and
   probably MacOS as far as I understand it correctly). Example:
   'VolumeName:Some:Path' or 'ServerName/Volume:Some\Path'
   (Netware).

   FPC_FEXPAND_NO_DEFAULT_PATHS - Dos keeps information about the
   current directory for every drive. If some platform supports
   drives or volumes, but keeps no track of current directories for
   them (i.e. there's no support for "GetDir(DriveNumber, Dir)" or
   "GetDir(Volume, Dir)", but only for "GetDir (0, Dir)" (i.e. the
   overall current directory), you should define this. Otherwise
   constructs like 'C:Some\Path' refer a path relative to the
   current directory on the C: drive.

   FPC_FEXPAND_DRIVESEP_IS_ROOT - this means that DriveSeparator
   should be used as beginning of the "real" path for a particular
   drive or volume instead of the DirectorySeparator. This would be
   used in case that there is only one character (DriveSeparator)
   delimitting the drive letter or volume name from the remaining
   path _and_ the DriveSeparator marks the root of an absolute path
   in that case. Example - 'Volume:This/Is/Absolute/Path'.

   FPC_FEXPAND_NO_CURDIR - there is no support to refer to current
   directory explicitely (like '.' used under both Unix and DOS-like
   environments).

   FPC_FEXPAND_NO_DOTS_UPDIR - '..' cannot be used to refer to the
   upper directory.

   FPC_FEXPAND_DIRSEP_IS_UPDIR - DirectorySeparator at the beginning of
   a path (or doubled DirectorySeparator inside the path) refer to the
   parent directory, one more DirectorySeparator to parent directory of
   parent directory and so on (Amiga). Please, note that you can decide
   to support both '..' and DirectorySeparator as references to the parent
   directory at the same time for compatibility reasons - however this
   support makes it impossible to use anotherwise possibly valid name
   of '..'.

   FPC_FEXPAND_DIRSEP_IS_CURDIR - DirectorySeparator at the beginning of
   a path refers to the current directory (i.e. path beginning with
   DirectorySeparator is always a relative path). Two DirectorySeparator
   characters refer to the parent directory, three refer to parent
   directory of the parent directory and so on (MacOS).

   FPC_FEXPAND_MULTIPLE_UPDIR - grouping of more characters specifying
   upper directory references higher directory levels. Example: '...'
   (Netware).

   FPC_FEXPAND_SYSUTILS allows to reuse the same implementation for
   SysUtils.ExpandFileName by avoiding things specific for unit Dos.
*)

