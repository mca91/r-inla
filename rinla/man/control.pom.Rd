%%
%% WARNING! DO NOT EDIT!
%% This file is automatically generated from set.default.arguments.R
%%
\name{control.pom}
\alias{control.pom}
\alias{inla.control.pom}
\alias{inla.set.control.pom.default}
\alias{set.control.pom.default}
\alias{control.pom.default}
\title{Control variables in control.pom}
\description{Control variables in \code{control.pom} for use in \code{inla}}
\usage{
inla.set.control.pom.default(...)
control.pom(cdf, fast)
}
\arguments{
\item{...}{Possible arguments}
\item{cdf}{   character The cdf to use, "logit" (default) or "probit"}
\item{fast}{   Logical Use a faster but approximate form for the probit cdf (default \code{FALSE})?}
}
\value{
   The \code{control.pom} -list is set within the corresponding \code{control.family}
The function \code{control.pom} is used to TAB-complete arguments and returns a list of given arguments.
The function \code{inla.set.control.pom.default} returns a list with all the default values of all parameters within this control statement.
}
\seealso{
\code{\link{control.update}}, \code{\link{control.lincomb}}, \code{\link{control.group}}, \code{\link{control.mix}}, \code{\link{control.pom}}, \code{\link{control.link}}, \code{\link{control.expert}}, \code{\link{control.compute}}, \code{\link{control.lp.scale}}, \code{\link{control.pardiso.default}}, \code{\link{control.bgev.default}}, \code{\link{control.family}}, \code{\link{control.fixed}}, \code{\link{control.inla}}, \code{\link{control.predictor}}, \code{\link{control.mode}}, \code{\link{control.hazard}}, 
\code{\link{inla}}
}
