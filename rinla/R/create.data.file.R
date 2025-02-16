## Nothing to Export

`inla.create.data.file` <- function(
                                    y.orig = NULL,
                                    MPredictor = NULL,
                                    mf = NULL,
                                    scale = NULL,
                                    weights = NULL,
                                    E = NULL,
                                    Ntrials = NULL,
                                    strata = NULL,
                                    lp.scale = NULL,
                                    event = NULL,
                                    family = NULL,
                                    data.dir = NULL,
                                    file = NULL,
                                    debug = FALSE) {
    y.attr <- attr(y.orig, "inla.ncols", exact = TRUE)
    if (is.null(y.attr)) {
        y.attr <- 0
    }

    if (is.null(y.orig)) {
        y.orig <- c(mf[, 1L])
    } else if (is.inla.surv(y.orig)) {
        ## this is not passed into the inla-program
        y.orig$.special <- NULL
        y.orig <- as.data.frame(unclass(y.orig))
    } else if (is.inla.mdata(y.orig)) {
        y.orig <- as.data.frame(unclass(y.orig))
    } else {
        y.orig <- as.data.frame(y.orig)
    }
    n.data <- dim(y.orig)[1L]
    ind <- seq(0L, n.data - 1L)

    if (debug) {
        cat("inla.create.data.file: n.data = ", n.data, "\n")
    }

    if (!is.null(weights) && !is.function(weights)) {
        if (length(weights) == 1L) {
            weights <- rep(weights, n.data)
        }
        if (length(weights) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of 'weights' has to be the same as the length of the response:", length(weights), n.data))
        }
    }

    if (!is.null(lp.scale)) {
        if (length(lp.scale) == 1L) {
            lp.scale <- rep(lp.scale, n.data)
        }
        if (length(lp.scale) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of 'lp.scale' has to be the same as the length of the response:", length(lp.scale), n.data))
        }
        lp.scale <- as.numeric(lp.scale)
        lp.scale[is.na(lp.scale)] <- 0
        lp.scale[lp.scale < 0] <- 0
        lp.max <- max(lp.scale)
    }

    if (inla.one.of(family, c(
        "gaussian",
        "normal",
        "lognormal",
        "t",
        "sn",
        "gev",
        "logistic",
        "circularnormal",
        "wrappedcauchy",
        "iidgamma",
        "simplex",
        "gamma",
        "beta",
        "tweedie",
        "fmri"))) {
        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        if (length(scale) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of scale has to be the same as the length of the response:", length(scale), n.data))
        }

        response <- cbind(ind, scale, y.orig)
        null.dat <- is.na(response[, 3L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("tstrata"))) {
        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        if (is.null(strata)) {
            strata <- rep(1L, n.data)
        }
        if (length(strata) == 1L) {
            strata <- rep(strata, n.data)
        }

        stopifnot(all(!is.na(strata)))
        stopifnot(all(as.integer(strata) == strata))
        stopifnot(all(strata > 0L))
        if (length(scale) != n.data || length(strata) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste(
                "Length of scale and strata has to be the same as the length of the response:",
                length(scale), length(strata), n.data
            ))
        }

        ## strata goes from 0L to nstrata-1L,  therefore subtract 1L.
        response <- cbind(ind, scale, as.integer(strata) - 1L, y.orig)
        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c(
        "poisson",
        "cenpoisson",
        "gammacount",
        "gpoisson",
        "xpoisson",
        "zeroinflatedcenpoisson0",
        "zeroinflatedcenpoisson1",
        "zeroinflatednbinomial0",
        "zeroinflatednbinomial1",
        "zeroinflatednbinomial2",
        "zeroinflatedpoisson0",
        "zeroinflatedpoisson1",
        "zeroinflatedpoisson2",
        "poisson.special1"))) {
        if (is.null(E)) {
            E <- rep(1.0, n.data)
        }
        if (length(E) == 1L) {
            E <- rep(E, n.data)
        }

        response <- cbind(ind, E, y.orig)

        if (length(E) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of E has to be the same as the length of the response:", length(E), n.data))
        }

        null.dat <- is.na(response[, 3L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("nbinomial"))) {
        if (is.null(E)) {
            E <- rep(1.0, n.data)
        }
        if (length(E) == 1L) {
            E <- rep(E, n.data)
        }

        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        response <- cbind(ind, E, scale, y.orig)

        if (length(E) != n.data || length(scale) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of E and scale has to be the same as the length of the response: E scale data ", length(E), length(scale), n.data))
        }
        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("exponential", "weibull", "loglogistic", "gammajw", "gompertz"))) {
        response <- cbind(ind, y.orig)
        null.dat <- is.na(response[, 2L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("zeroinflatednbinomial1strata2", "zeroinflatednbinomial1strata3"))) {
        if (is.null(E)) {
            E <- rep(1.0, n.data)
        }
        if (length(E) == 1L) {
            E <- rep(E, n.data)
        }

        if (is.null(strata)) {
            strata <- rep(1L, n.data)
        }
        if (length(strata) == 1L) {
            strata <- rep(strata, n.data)
        }

        stopifnot(all(!is.na(strata)))
        stopifnot(all(strata %in% 1:10))

        response <- cbind(ind, E, strata - 1L, y.orig)

        if (length(E) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste(
                "Length of E has to be the same as the length of the response:",
                length(E), n.data
            ))
        }

        if (length(strata) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste(
                "Length of strata has to be the same as the length of the response:",
                length(strata), n.data
            ))
        }

        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("xbinomial"))) {
        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        if (is.null(Ntrials)) {
            Ntrials <- rep(1L, n.data)
        }
        if (length(Ntrials) == 1L) {
            Ntrials <- rep(Ntrials, n.data)
        }

        response <- cbind(ind, Ntrials, scale, y.orig)
        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(
        family,
        c(
            "binomial",
            "binomialtest",
            "betabinomial",
            "nbinomial2",
            "zeroinflatedbinomial0",
            "zeroinflatedbinomial1",
            "zeroinflatedbinomial2",
            "zeroninflatedbinomial2",
            "zeroninflatedbinomial3",
            "zeroinflatedbetabinomial0",
            "zeroinflatedbetabinomial1",
            "zeroinflatedbetabinomial2"
        )
    )) {
        if (is.null(Ntrials)) {
            Ntrials <- rep(1L, n.data)
        }
        if (length(Ntrials) == 1L) {
            Ntrials <- rep(Ntrials, n.data)
        }

        response <- cbind(ind, Ntrials, y.orig)
        null.dat <- is.na(response[, 3L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("betabinomialna"))) {
        if (is.null(Ntrials)) {
            Ntrials <- rep(1L, n.data)
        }
        if (length(Ntrials) == 1L) {
            Ntrials <- rep(Ntrials, n.data)
        }

        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        if (length(scale) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of scale has to be the same as the length of the response:", length(scale), n.data))
        }

        response <- cbind(ind, Ntrials, scale, y.orig)
        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("cbinomial"))) {
        if (!(is.matrix(Ntrials) && all(dim(Ntrials) == c(n.data, 2)))) {
            stop(paste(
                "Argument 'Ntrials' for family='cbinomial' must be a", n.data,
                "x", 2, "-matrix; see the documentation."
            ))
        }
        response <- cbind(ind, Ntrials, y.orig)
        null.dat <- is.na(response[, 4L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c(
        "exponentialsurv", "weibullsurv", "weibullcure",
        "loglogisticsurv", "qloglogisticsurv", "lognormalsurv",
        "gammasurv", "gammajwsurv", "fmrisurv", "gompertzsurv"))) {
        if (!inla.model.properties(family, "likelihood")$survival) {
            file.remove(file)
            file.remove(data.dir)
            stop("This should not happen.")
        }
        if (is.null(y.orig$time)) {
            file.remove(file)
            file.remove(data.dir)
            stop("Responce does not contain variable `time'.")
        }
        len <- length(y.orig$time)

        if (is.null(y.orig$truncation)) {
            y.orig$truncation <- rep(0, len)
        }
        if (is.null(y.orig$lower)) {
            y.orig$lower <- rep(0, len)
        }
        if (is.null(y.orig$upper)) {
            y.orig$upper <- rep(Inf, len)
        }
        if (is.null(y.orig$event)) {
            y.orig$event <- rep(1, len)
        }

        idx <- !is.na(y.orig$time)
        response <- cbind(
            ind[idx],
            y.orig$event[idx],
            y.orig$truncation[idx],
            y.orig$lower[idx],
            y.orig$upper[idx],
            y.orig$time[idx]
        )
        if (any(is.na(response))) {
            file.remove(file)
            file.remove(data.dir)
            stop("NA in truncation/event/lower/upper/time is not allowed")
        }
    } else if (inla.one.of(family, c(
        "stochvol", "stochvolt", "stochvolnig", "stochvolsn", "loggammafrailty",
        "iidlogitbeta", "qkumar", "qloglogistic", "gp", "dgp", "pom",
        "logperiodogram"))) {
        response <- cbind(ind, y.orig)
        null.dat <- is.na(response[, 2L])
        response <- response[!null.dat, ]
    } else if (inla.one.of(family, c("nmix", "nmixnb"))) {
        if (inla.one.of(family, "nmix")) {
            mmax <- length(inla.model.properties(model = family, section = "likelihood")$hyper)
        } else if (inla.one.of(family, "nmixnb")) {
            ## remove the overdispersion parameter
            mmax <- length(inla.model.properties(model = family, section = "likelihood")$hyper) - 1
        } else {
            stop("This should not happen.")
        }

        response <- cbind(IDX = ind, y.orig)
        col.idx <- grep("^IDX$", names(response))
        col.x <- grep("^X[0-9]+", names(response))
        col.y <- grep("^Y[0-9]+", names(response))
        m.x <- length(col.x)
        m.y <- length(col.y)
        stopifnot(m.x >= 1 && m.x <= mmax)

        ## remove entries with NA's in all responses
        na.y <- apply(response[, col.y, drop = FALSE], 1, function(x) all(is.na(x)))
        response <- response[!na.y, , drop = FALSE]

        X <- response[, col.x, drop = FALSE]
        Y <- response[, col.y, drop = FALSE]
        idx <- response[, col.idx, drop = FALSE]
        yfake <- rep(-1, nrow(Y))

        ## replace NA's in the covariates with 0's
        X[is.na(X)] <- 0
        ## augment X til the maximum allowed,  padding with NA's
        X <- cbind(X, matrix(NA, nrow = nrow(response), ncol = mmax - m.x))

        ## sort each row so that the NA's are at the end. Sort numerically the non-NA's as well
        ## although it is not required
        Y <- matrix(c(apply(Y, 1, function(x) c(sort(x[!is.na(x)]), x[is.na(x)]))), ncol = ncol(Y), byrow = TRUE)
        response <- cbind(idx, X, Y, yfake)
    } else if (inla.one.of(family, c("agaussian"))) {
        response <- cbind(IDX = ind, y.orig)
        col.idx <- grep("^IDX$", names(response))
        col.y <- grep("^Y[0-9]+", names(response))
        m.y <- length(col.y)
        stopifnot(m.y == 5L)
        ## remove entries with NA's in all responses
        na.y <- apply(response[, col.y, drop = FALSE], 1, function(x) all(is.na(x)))
        response <- response[!na.y, , drop = FALSE]
        Y <- response[, col.y, drop = FALSE]
        idx <- response[, col.idx, drop = FALSE]
        response <- cbind(idx, Y)
    } else if (inla.one.of(family, c("cenpoisson2"))) {

        if (is.null(E)) {
            E <- rep(1, n.data)
        }
        if (length(E) == 1L) {
            E <- rep(E, n.data)
        }

        response <- cbind(ind, E, y.orig)
        stopifnot(ncol(response) == 5)
        null.dat <- is.na(response[, 3L])
        response <- response[!null.dat, ]
        colnames(response) <- c("IDX", "E", "Y1", "Y2", "Y3")
        idx.inf <- (is.infinite(response$Y3) | (response$Y3 < 0))
        response[idx.inf, "Y3"] <- -1 ## code for infinite
        idx.inf <- (is.infinite(response$Y2) | (response$Y2 < 0))
        response[idx.inf, "Y2"] <- -1 ## code for infinite
        col.idx <- grep("^IDX$", names(response))
        col.y <- grep("^Y[0-9]+", names(response))
        m.y <- length(col.y)
        stopifnot(m.y == 3L)
        ## remove entries with NA's in all responses
        na.y <- apply(response[, col.y, drop = FALSE], 1, function(x) all(is.na(x)))
        response <- response[!na.y, , drop = FALSE]
        ## format: IDX, E, LOW, HIGH, Y
        response <- cbind(IDX = response$IDX, E = response$E, LOW = response$Y2, HIGH = response$Y3, Y = response$Y1)
    } else if (inla.one.of(family, c("bgev"))) {
        if (is.null(scale)) {
            scale <- rep(1.0, n.data)
        }
        if (length(scale) == 1L) {
            scale <- rep(scale, n.data)
        }

        if (length(scale) != n.data) {
            file.remove(file)
            file.remove(data.dir)
            stop(paste("Length of scale has to be the same as the length of the response:", length(scale), n.data))
        }

        mmax <- length(inla.model.properties(model = family, section = "likelihood")$hyper) - 2L
        response <- cbind(IDX = ind, y.orig)
        col.idx <- grep("^IDX$", names(response))
        col.x <- grep("^X[0-9]+", names(response))
        col.y <- grep("^Y[0-9]+", names(response))
        m.x <- length(col.x)
        m.y <- length(col.y)

        ## remove entries with NA's in all responses
        na.y <- apply(response[, col.y, drop = FALSE], 1, function(x) all(is.na(x)))
        response <- response[!na.y, , drop = FALSE]
        scale <- scale[!na.y]

        X <- response[, col.x, drop = FALSE]
        Y <- response[, col.y, drop = FALSE]
        stopifnot(ncol(Y) == 1)
        idx <- response[, col.idx, drop = FALSE]
        ## replace NA's in the covariates with 0's
        X[is.na(X)] <- 0

        response <- cbind(idx, scale, X, Y)
        ## fix attr, so the order corresponds to (X, Y) and not (Y, X) as in the inla.mdata() input.
        ## y.attr[1] is number of attributes
        stopifnot(y.attr[1] > 0)
        if (y.attr[1] == 1) {
            y.attr <- c(3, y.attr[2], 0, 0)
        } else if (y.attr[1] == 2) {
            y.attr <- c(3, y.attr[2], y.attr[3], 0)
        } else if (y.attr[1] == 3) {
            y.attr <- c(3, y.attr[2], y.attr[3], y.attr[4])
        }
        y.attr <- c(y.attr[1], y.attr[-c(1, 2)], y.attr[2])
    } else {
        file.remove(file)
        file.remove(data.dir)
        stop(paste("Family", family, ", not recognised in 'create.data.file.R'"))
    }

    file.data <- inla.tempfile(tmpdir = data.dir)
    inla.write.fmesher.file(as.matrix(response), filename = file.data, debug = debug)
    file.data <- gsub(data.dir, "$inladatadir", file.data, fixed = TRUE)

    file.weights <- inla.tempfile(tmpdir = data.dir)
    if (!is.null(weights) && !is.function(weights)) {
        inla.write.fmesher.file(as.matrix(weights), filename = file.weights, debug = debug)
    } else {
        file.create(file.weights)
    }
    file.weights <- gsub(data.dir, "$inladatadir", file.weights, fixed = TRUE)

    file.lp.scale <- inla.tempfile(tmpdir = data.dir)
    if (!is.null(lp.scale)) {
        inla.write.fmesher.file(as.matrix(lp.scale), filename = file.lp.scale, debug = debug)
    } else {
        file.create(file.lp.scale)
    }
    file.lp.scale <- gsub(data.dir, "$inladatadir", file.lp.scale, fixed = TRUE)

    file.attr <- inla.tempfile(tmpdir = data.dir)
    inla.write.fmesher.file(as.matrix(y.attr, ncol = 1), filename = file.attr, debug = debug)
    file.attr <- gsub(data.dir, "$inladatadir", file.attr, fixed = TRUE)

    return(list(file.data = file.data, file.weights = file.weights, file.attr = file.attr,
                file.lp.scale = file.lp.scale))
}
