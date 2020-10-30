/-
A Euclidean Geometry 3-Space is constructed.	
The standard frame on the space is referenced, using SI and bound to a variable.
The standard frame on the space is referenced, again, using SI, and bound to a variable.
One point variable is created and is assigned to a literal in the space, using the standard frame
One vector variable is created and is assigned to a literal in the space, using the standard frame
These points are subtracted. An error occurs.
-/

import .lang.imperative_DSL.physlang

noncomputable theory

def env7 := init_env


def worldGeometry := cmd.classicalGeometryAssmt (lang.classicalGeometry.var.mk 1) (lang.classicalGeometry.expr.lit(classicalGeometry.mk 1 3))

 def env8 := cmdEval worldGeometry env7

def stdWorldFrame := cmd.classicalGeometryFrameAssmt (lang.classicalGeometry.frame_var.mk 11) (lang.classicalGeometry.getStdFrame (classicalGeometryEval (lang.classicalGeometry.expr.var (lang.classicalGeometry.var.mk 1)) ( env8 )))

def env9 := cmdEval worldGeometry.stdFrame env8

def worldTime := cmd.classicalTimeAssmt (lang.classicalTime.var.mk 3) (lang.classicalTime.expr.lit(classicalTime.mk 2))

def env10 := cmdEval worldTime env9

----def worldTime.stdFrame := cmd.classicalGeometryFrameAssmt (lang.classicalGeometry.frame_var.mk 12) (lang.classicalTime.getStdFrame (classicalTimeEval (lang.classicalTime.expr.var (lang.classicalTime.var.mk 2)) ( env10 )))

def env11 := cmdEval worldTime.stdFrame env10

def worldVelocity := cmd.classicalVelocityAssmt 
		(lang.classicalVelocity.var.mk 5) 
		(lang.classicalVelocity.expr.lit (classicalVelocity.mk 4 
			(lang.classicalGeometry.eval (lang.classicalGeometry.expr.var (lang.classicalGeometry.var.mk 1)) (classicalGeometryGet env9) ) 
			(lang.classicalTime.eval (lang.classicalTime.expr.var (lang.classicalTime.var.mk 3)) (classicalTimeGet env9) )))

def env12 := cmdEval worldVelocity env11

--def worldVelocity.stdFrame := cmd.classicalVelocityFrameAssmt (lang.classicalVelocity.frame_var.mk 13) (lang.classicalVelocity.getStdFrame (classicalVelocityEval (lang.classicalVelocity.expr.var (lang.classicalVelocity.var.mk 2)) ( env12 )))

def env13 := cmdEval  worldVelocity.stdFrame env12

def INDEX104809424.STMTCOMMAND.B.L97C32.E.L110C1 : cmd :=(cmd.seq worldVelocity cmd.seq(cmd.seq worldTime (cmd.seq worldGeometry worldGeometry.stdFrame)))

