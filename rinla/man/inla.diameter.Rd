% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/mesh.R
\name{inla.diameter}
\alias{inla.diameter}
\alias{inla.diameter.default}
\alias{inla.diameter.inla.mesh}
\alias{inla.diameter.inla.mesh.segment}
\alias{inla.diameter.inla.mesh.lattice}
\alias{inla.diameter.inla.mesh.1d}
\title{Diameter of a point set}
\usage{
inla.diameter(x, ...)

\method{inla.diameter}{default}(x, manifold = "", ...)

\method{inla.diameter}{inla.mesh.1d}(x, ...)

\method{inla.diameter}{inla.mesh}(x, ...)

\method{inla.diameter}{inla.mesh.segment}(x, ...)

\method{inla.diameter}{inla.mesh.lattice}(x, ...)
}
\arguments{
\item{x}{A point set as an \eqn{n\times d}{n x d} matrix, or an
\code{inla.mesh} related object.}

\item{\dots}{Additional parameters passed on to other methods.}

\item{manifold}{Character string specifying the manifold type. Default is to
treat the point set with Euclidean \eqn{R^d} metrics. Use
\code{manifold="S2"} for great circle distances on the unit sphere (this is
set automatically for \code{inla.mesh} objects).}
}
\value{
A scalar, upper bound for the diameter of the convex hull of the
point set.
}
\description{
Find an upper bound to the convex hull of a point set
}
\details{
\itemize{
\item \code{inla.diameter.default}
Calculate upper bound for the diameter of a point set,
by encapsulating in a circular domain.
}
}
\examples{

inla.diameter(matrix(c(0, 1, 1, 0, 0, 0, 1, 1), 4, 2))
}
\author{
Finn Lindgren \href{mailto:finn.lindgren@gmail.com}{finn.lindgren@gmail.com}
}
