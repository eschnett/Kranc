
(*  Copyright 2012 Ian Hinder

    This file is part of Kranc.

    Kranc is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kranc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kranc; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*)

BeginPackage["Calculation`", {"Errors`", "Helpers`", "Kranc`", "KrancGroups`",
                              "MapLookup`","Differencing`"}];

InputGridFunctions;
OutputGridFunctions;
AllGridFunctions;
GetCalculationName;
GetEquations;
GetCalculationParameters;
CalculationStencilSize;
CalculationOnDevice;
GetCalculationWhere;
SplitCalculations;
SeparateDerivatives;

Begin["`Private`"];

DefFn[
  GetGridFunctions[calc_List] :=
  Module[
    {eqs,rhss,lhss,gfs,gfsInRHS,gfsInLHS},
    eqs = GetEquations[calc];

    rhss = Map[#[[2]] &, eqs];
    lhss = Map[#[[1]] &, eqs];

    gfs = allGroupVariables[lookup[calc,Groups]];

    gfsInRHS = Union[Cases[rhss, _ ? (MemberQ[gfs,#] &), Infinity]];
    gfsInLHS = Union[Cases[lhss, _ ? (MemberQ[gfs,#] &), Infinity]];

    {gfsInLHS, gfsInRHS}]];

DefFn[
  InputGridFunctions[calc_List] :=
  Last[GetGridFunctions[calc]]];

DefFn[
  OutputGridFunctions[calc_List] :=
  First[GetGridFunctions[calc]]];

DefFn[
  AllGridFunctions[calc_List] :=
  Union@@GetGridFunctions[calc]];

(* This is from the user's input calculation, not after processing/optimisation *)
DefFn[
  GetEquations[calc_List] :=
  lookup[calc,Equations]];

DefFn[
  GetPartialDerivatives[calc_List] :=
  lookup[calc,PartialDerivatives]];

DefFn[
  GetDerivatives[calc_] :=
  GridFunctionDerivativesInExpression[
    GetPartialDerivatives[calc], GetEquations[calc],{(* TODO: implement ZeroDimensions here *)}]];

DefFn[
  GetCalculationParameters[calc_List] :=
  Module[
    {syms,pdSyms,params},
    syms = Cases[GetEquations[calc], _?AtomQ|_String, {-1}];
    pdSyms = Cases[GetPartialDerivatives[calc], _?AtomQ|_String, {-1}];
    params = lookup[calc,Parameters];
    Intersection[params,Join[syms,pdSyms]]]];

DefFn[
  GetCalculationName[calc_List] :=
  lookup[calc,Name]];

DefFn[
  GetCalculationWhere[calc_List] :=
  lookup[calc,Where, Everywhere]];

DefFn[
  CalculationStencilSize[calc_List] :=
  Module[
    {pddefs,eqs},

    pddefs = lookup[calc, PartialDerivatives, {}];
    eqs    = lookup[calc, Equations];

    StencilSize[pddefs, eqs, "not needed", {} (*ZeroDimensions*)]]];

DefFn[
  CalculationOnDevice[calc_List] :=
  lookupDefault[calc, ExecuteOn, Automatic] === Device];

partialCalculation[calc_, suffix_, updates_, evolVars_] :=
Module[
  {name, calc1, replaces, calc2, vars, patterns, eqs, calc3},
  (* Add suffix to name *)
  name     = lookup[calc, Name] <> suffix;
  calc1    = mapReplace[calc, Name, name];
  (* Replace some entries in the calculation *)
  replaces = updates //. (lhs_ -> rhs_) -> (mapReplace[#, lhs, rhs]&);
  calc2 = Apply[Composition, replaces][calc1];
  (* Remove unnecessary equations *)
  vars     = Join[evolVars, lookup[calc2, Shorthands]];
  patterns = Replace[vars, {    Tensor[n_,__]  ->     Tensor[n,__] ,
                            dot[Tensor[n_,__]] -> dot[Tensor[n,__]]}, 1];
  eqs      = FilterRules[lookup[calc, Equations], patterns];
  calc3    = mapReplace[calc2, Equations, eqs];
  calc3
];

DefFn[
  SplitCalculations[calcs_List] :=
  Flatten[SplitCalculation/@calcs,1]];

(* Split a calculation into a set of equivalent calculations.  Any
   required shorthands are recomputed in each calculation.  The split
   is determined by the SplitVars calculation option, which is a list
   of split specifications.  A split specification can be a single
   variable, in which case the calculation will compute only that
   variable, and will be named oldName_<varname>, or it can be a list
   of variables, in which case the calculation will compute all those
   variables and will be named with a numeric index. *)
DefFn[
  SplitCalculation[calc_] :=
  Module[
    {splitBy = lookup[calc,SplitBy, {}],
     oldName = lookup[calc,Name]},

    If[Intersection[Flatten[splitBy,1],OutputGridFunctions[calc]] === {},
       {calc},
       MapIndexed[
         Function[
           {var,i},
           Module[
             {nameSuffix, splitVars},
             nameSuffix = If[ListQ[var],
                             ToString[i[[1]]],
                             "_"<>StringReplace[ToString[var],{"["->"","]"->"",","->""}]];
             splitVars = If[ListQ[var], var, {var}];
             partialCalculation[calc, nameSuffix, {}, splitVars]]],
         splitBy]]]];

DefFn[
  SeparateDerivatives[calcs_List] :=
  Flatten[separateDerivativesInCalculation/@calcs,1]];

(* If the calculation contains a SeparatedDerivatives key, split the
   calculation into two.  The first one will compute all the
   derivatives matching the SeparatedDerivatives pattern and store the
   results in grid functions.  The second will then use these grid
   functions instead of computing the derivatives. *)

separateDerivativesInCalculation[calc_] :=
  Module[
    {sepPat = lookup[calc,SeparatedDerivatives, None]},
    If[sepPat === None, {calc},
       If[lookupDefault[calc, Schedule, Automatic] === Automatic,
          ThrowError["Separating derivatives in an automatically scheduled function is not supported"]];

       Module[
         {derivGFName, derivs, sepDerivs, calc2, replaceSymmetric},
         derivGFName[pd_[var_,inds___]] :=
         Symbol["Global`D"<>ToString[pd]<>ToString[var]<>Apply[StringJoin,Map[ToString,{inds}]]];

         replaceSymmetric = pd_[var_,i_,j_] /; i > j :> pd[var,j,i];
         derivs = DeleteDuplicates[GetDerivatives[calc] /. replaceSymmetric];
         sepDerivs = Flatten[Map[Cases[derivs, #] &, sepPat],1];
         
         derivCalc[sepDeriv_] :=
         Module[
           {calc1, currentGroups, localGroups, derivName = derivGFName[sepDeriv]},
           calc1 = mapReplace[calc, 
                              Equations, 
                              {derivName -> sepDeriv}];
           calc1 = mapReplace[calc1, Schedule, Map[#<>" before "<>lookup[calc,Name] &, lookup[calc,Schedule]]];
           calc1 = mapReplace[calc1, Name, lookup[calc,Name]<>"_"<>ToString@derivName];
           currentGroups = lookup[calc, LocalGroups, {}];
           localGroups = Append[currentGroups, {ToString@derivName<>"_group", {derivName}}];
           calc1 = mapReplaceAdd[calc1, LocalGroups, localGroups];
           calc1];

         derivCalcs = Map[derivCalc, sepDerivs];

         calc2 = mapReplace[calc,
                            Equations,
                            (GetEquations[calc]/.replaceSymmetric) /. Map[# -> derivGFName[#] &, sepDerivs]];

         Append[derivCalcs, calc2]]]];

End[];

EndPackage[];