/**
 * @file  ec_p256.c  Implementation curve arithmetic (NIST-P256) for ECC.
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/Util.h>
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCp256.h>

namespace qcc {

#define W_VARBASE 6     /* Parameter for scalar multiplication.  Should use 2-2.5 KB.  Must be >= 2. */

static digit256_tc P256_A = { 18446744073709551612U, 4294967295U, 0U, 18446744069414584321U };
static digit256_tc P256_B = { 4309448131093880907U, 7285987128567378166U, 12964664127075681980U, 6540974713487397863U };
static digit256_tc P256_ORDER = { 17562291160714782033U, 13611842547513532036U, 18446744073709551615U, 18446744069414584320U };
static digit256_tc P256_GENERATOR_X = { 17627433388654248598U, 8575836109218198432U, 17923454489921339634U, 7716867327612699207U };
static digit256_tc P256_GENERATOR_Y = { 14678990851816772085U, 3156516839386865358U, 10297457778147434006U, 5756518291402817435U };

QStatus ec_getcurve(ec_t* curve, curveid_t curveid)
{
    QStatus status = ER_FAIL;

    if (curve == NULL) {
        status = ER_INVALID_ADDRESS;
        goto Exit;
    }

    if (curveid == NISTP256r1) {
        memset(curve, 0x00, sizeof(ec_t));

        curve->curveid = curveid;
        curve->rbits = 256;
        curve->pbits = 256;

        curve->prime = (digit_t*)malloc(sizeof(digit256_t));
        curve->a = (digit_t*)malloc(sizeof(digit256_tc));
        curve->b = (digit_t*)malloc(sizeof(digit256_tc));
        curve->order = (digit_t*)malloc(sizeof(digit256_tc));
        if ((curve->prime == NULL) || (curve->a == NULL) || (curve->b == NULL) || (curve->order == NULL)) {
            status = ER_OUT_OF_MEMORY;
            goto Exit;
        }

        fpgetprime_p256(curve->prime);
        fpcopy_p256(P256_A, curve->a);
        fpcopy_p256(P256_B, curve->b);
        fpcopy_p256(P256_ORDER, curve->order);
        fpcopy_p256(P256_GENERATOR_X, curve->generator.x);
        fpcopy_p256(P256_GENERATOR_Y, curve->generator.y);

        /* These two curve fields are required for ECDSA, Montgomery arithmetic modulo the group order.  Precompute and set here.
         *    digit_t*         Rprime;                            (2^W)^2 mod r, where r is the order and W is its bitlength
         *    digit_t*         rprime;                            -(r^-1) mod 2^W
         */

        status = ER_OK;
    } else {   /* Unknown curve.  */
        status = ER_BAD_ARG_2;
        goto Exit;
    }

Exit:

    if (status != ER_OK) {
        ec_freecurve(curve);
    }

    return status;
}

void ec_freecurve(ec_t* curve)
{
    if (curve != NULL) {
        // Cleanup
        free(curve->a);
        free(curve->b);
        free(curve->order);
        free(curve->prime);
        memset(curve, 0x00, sizeof(ec_t));
    }
}

/* Convert affine point Q = (x,y) to Jacobian P = (X:Y:1), where X=x, Y=y */
void ec_affine_tojacobian(const ecpoint_t* Q, ecpoint_jacobian_t* P)
{
    assert(Q != NULL);
    assert(P != NULL);

    fpcopy_p256(Q->x, P->X);
    fpcopy_p256(Q->y, P->Y);
    fpzero_p256(P->Z);
    P->Z[0] = 1;
}

/* Set P to the generator of the curve.  */
void ec_get_generator(ecpoint_t* P, ec_t* curve)
{
    assert(curve != NULL);

    memcpy(P->x, curve->generator.x, sizeof(digit256_t));
    memcpy(P->y, curve->generator.y, sizeof(digit256_t));
}

/* Set the jacobian point P to zero (0,0,0). */
static void ecpoint_jacobian_zero(ecpoint_jacobian_t* P)
{
    assert(P != NULL);
    fpzero_p256(P->X);
    fpzero_p256(P->Y);
    fpzero_p256(P->Z);
}

/* Copy Jacobian points, set src = dst. */
static void ecpoint_jacobian_copy(ecpoint_jacobian_t* src, ecpoint_jacobian_t* dst)
{
    assert(src != NULL);
    assert(dst != NULL);

    fpcopy_p256(src->X, dst->X);
    fpcopy_p256(src->Y, dst->Y);
    fpcopy_p256(src->Z, dst->Z);
}

static void ecpoint_chudnovsky_zero(ecpoint_chudnovsky_t* P)
{
    assert(P != NULL);
    fpzero_p256(P->X);
    fpzero_p256(P->Y);
    fpzero_p256(P->Z);
    fpzero_p256(P->Z2);
    fpzero_p256(P->Z3);
}

/* Check if point P is the point at infinity (0,0) */
boolean_t ec_is_infinity(const ecpoint_t* P, ec_t* curve)
{
    size_t i = 0;
    digit_t c = 0;
    size_t num_digits = NBITS_TO_NDIGITS(curve->pbits);

    for (i = 0; i < num_digits; i++) {
        c = c | P->x[i] | P->y[i];
    }

    return is_digit_zero_ct(c);
}

/* Check if Jacobian point P is the point at infinity (0:Y:0) */
boolean_t ec_is_infinity_jacobian(const ecpoint_jacobian_t* P, ec_t* curve)
{
    size_t i = 0;
    digit_t c = 0;
    size_t num_digits = NBITS_TO_NDIGITS(curve->pbits);

    for (i = 0; i < num_digits; i++) {
        c = c | P->X[i] | P->Z[i];
    }

    return is_digit_zero_ct(c);
}

boolean_t ec_oncurve(const ecpoint_t* P, ec_t* curve)
{
    digit256_t t1, t2, t3;
    digit_t temps[P256_TEMPS];
    boolean_t oncurve;

    QCC_UNUSED(curve);

    /* Do (x,y) satisfy the curve equation y^2 = x^3 -3x + b (mod p) */
    fpsqr_p256(P->y, t1, temps);        /* t1 = y^2 */
    fpsqr_p256(P->x, t2, temps);        /* t2 = x^2 */
    fpmul_p256(P->x, t2, t2, temps);    /* t2 = x^3 */
    fpadd_p256(t2, P256_B, t2);         /* t2 = x^3 +b */
    fpadd_p256(P->x, P->x, t3);         /* t3 = 2x*/
    fpadd_p256(P->x, t3, t3);           /* t3 = 3x */
    fpsub_p256(t2, t3, t2);             /* t2 = x^3 - 3x + b */
    oncurve = fpequal_p256(t1, t2);

    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    ClearMemory(temps, sizeof(temps));

    return oncurve;
}

/* Check that P=(x,y) lies on the curve, is nonzero has x and y in [0, p-1]. */
boolean_t ecpoint_validation(const ecpoint_t* P, ec_t* curve)
{
    if (ec_is_infinity(P, curve)) {
        return B_FALSE;
    }
    if (!fpvalidate_p256(P->x) || !fpvalidate_p256(P->y)) {
        return B_FALSE;
    }
    if (!ec_oncurve(P, curve)) {
        return B_FALSE;
    }

    return B_TRUE;
}

/* Convert the Jacobian point Q = (X:Y:Z) to an affine point P = (x,y) */
void ec_toaffine(ecpoint_jacobian_t* Q, ecpoint_t* P, ec_t* curve)
{
    digit256_t t1, t2, t3;
    digit_t temps[P256_TEMPS];

    /* Check if Q is the point at infinity (0:Y:0) */
    /* SECURITY NOTE: this if-statement evaluates over public information when the function is called from constant-time scalar multiplications, i.e.,
     *               Q is never the point at infinity when the call is from ec_scalarmul().
     */
    if (ec_is_infinity_jacobian(Q, curve)) {
        fpzero_p256(P->x);
        fpzero_p256(P->y);    /* Output the point at infinity P = (0,0) */
        return;
    }

    fpinv_p256(Q->Z, t1, temps);                /* t1 = Z^-1  */
    fpsqr_p256(t1, t2, temps);                  /* t2 = Z^-2  */
    fpmul_p256(Q->X, t2, t3, temps);            /* t3 = X/Z^2 */
    fpcopy_p256(t3, P->x);                      /* x = X/Z^2  */
    fpmul_p256(t1, t2, t3, temps);              /* t3 = Z^-3  */
    fpmul_p256(Q->Y, t3, t1, temps);            /* t1 = Y/Z^3 */
    fpcopy_p256(t1, P->y);                      /* y = Y/Z^3  */

    /* cleanup */
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    ClearMemory(temps, sizeof(temps));
}

/* Point doubling P = 2P
 * Weierstrass a=-3 curve
 * Input:  P = (X,Y,Z) in Jacobian coordinates
 * Output: 2P = (X,Y,Z) in Jacobian coordinates
 */
void ec_double_jacobian(ecpoint_jacobian_t* P)
{
    digit256_t t1, t2, t3, t4;
    digit_t temps[P256_TEMPS];

    /* SECURITY NOTE: this function does not produce exceptions on prime-order Weierstrass curves (such as NIST P256). */

    fpsqr_p256(P->Z, t1, temps);          /* t1 = z^2  */
    fpmul_p256(P->Z, P->Y, t4, temps);    /* t4 = zy  */
    fpadd_p256(P->X, t1, t2);             /* t2 = x + z^2  */
    fpsub_p256(P->X, t1, t1);             /* t1 = x - z^2  */
    fpcopy_p256(t4, P->Z);                /* Zfinal = zy  */
    fpmul_p256(t1, t2, t3, temps);        /* t3 = (x + z^2)(x - z^2)  */
    fpdiv2_p256(t3, t2, temps);           /* t2 = (x + u.z^2)(x - u.z^2)/2  */
    fpadd_p256(t3, t2, t1);               /* t1 = alpha = 3(x + u.z^2)(x - u.z^2)/2  */
    fpsqr_p256(P->Y, t2, temps);          /* t2 = y^2  */
    fpsqr_p256(t1, t4, temps);            /* t4 = alpha^2  */
    fpmul_p256(P->X, t2, t3, temps);      /* t3 = beta = xy^2  */
    fpsub_p256(t4, t3, t4);               /* t4 = alpha^2-beta  */
    fpsub_p256(t4, t3, P->X);             /* Xfinal = alpha^2-2beta  */
    fpsub_p256(t3, P->X, t4);             /* t4 = beta-Xfinal  */
    fpsqr_p256(t2, t3, temps);            /* t3 = y^4  */
    fpmul_p256(t1, t4, t2, temps);        /* t2 = alpha.(beta-Xfinal)  */
    fpsub_p256(t2, t3, P->Y);             /* Yfinal = alpha.(beta-Xfinal)-y^4  */

    /* cleanup */
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    fpzero_p256(t4);
    ClearMemory(temps, sizeof(temps));
}

/* Point addition P = 2P+Q
 * Weierstrass a=-3 curve
 * Inputs: P = (X1,Y1,Z1) in Jacobian coordinates
 *         Q = (X2,Y2,Z2,Z2^2,Z2^3) in Chudnovsky coordinates
 * Output: P = (X1,Y1,Z1) in Jacobian coordinates
 */
void ec_doubleadd(ecpoint_chudnovsky_t* Q, ecpoint_jacobian_t* P, ec_t* curve)
{
    digit256_t t1, t2, t3, t4, t5, t6, t7;
    digit_t temps[P256_TEMPS];

    /*  SECURITY NOTE: this function does not produce exceptions when P!=inf, Q!=inf, P!=Q, P!=-Q or Q!=-2P.
     *  In particular, it is exception-free when called from ec_scalarmul().
     */

    QCC_UNUSED(curve);

    fpsqr_p256(P->Z, t2, temps);            /* t2 = z1^2  */
    fpmul_p256(Q->Z3, P->Y, t3, temps);     /* t3 = z2^3*y1  */
    fpmul_p256(P->Z, t2, t4, temps);        /* t4 = z1^3  */
    fpmul_p256(t2, Q->X, t1, temps);        /* t1 = z1^2*x2  */
    fpmul_p256(Q->Y, t4, t2, temps);        /* t2 = z1^3*y2  */
    fpmul_p256(Q->Z2, P->X, t6, temps);     /* t6 = z2^2*x1  */
    fpsub_p256(t2, t3, t2);                 /* t2 = alpha = z1^3*y2-z2^3*y1  */
    fpsub_p256(t1, t6, t1);                 /* t1 = beta = z1^2*x2-z2^2*x1  */
    fpsqr_p256(t2, t4, temps);              /* t4 = alpha^2  */
    fpsqr_p256(t1, t5, temps);              /* t5 = beta^2  */
    fpmul_p256(P->Z, Q->Z, t7, temps);      /* t5 = z1*z2  */
    fpmul_p256(t6, t5, P->X, temps);        /* x1 = x1' = z2^2*x1*beta^2  */
    fpmul_p256(t1, t5, t6, temps);          /* t6 = beta^3  */
    fpsub_p256(t4, t6, t4);                 /* t4 = alpha^2 - beta^3  */
    fpsub_p256(t4, P->X, t4);               /* t4 = alpha^2 - beta^3 - x1'  */
    fpsub_p256(t4, P->X, t4);               /* t4 = alpha^2 - beta^3 - 2*x1'  */
    fpsub_p256(t4, P->X, t4);               /* t4 = omega = alpha^2 - beta^3 - 3*x1'  */
    fpmul_p256(t6, t3, P->Y, temps);        /* y1 = y1' = z2^3*y1*beta^3  */
    fpmul_p256(t1, t7, t3, temps);          /* t3 = z1' = z1*z2*beta  */
    fpmul_p256(t2, t4, t1, temps);          /* t1 = alpha.omega  */
    fpsqr_p256(t4, t2, temps);              /* t2 = omega^2  */
    fpadd_p256(t1, P->Y, t1);               /* t1 = alpha.omega + y1'  */
    fpadd_p256(t1, P->Y, t1);               /* t1 = theta = alpha.omega + 2y1'  */
    fpmul_p256(t3, t4, P->Z, temps);        /* Zfinal = z1'*omega  */
    fpmul_p256(t2, t4, t5, temps);          /* t5 = omega^3  */
    fpmul_p256(t2, P->X, t4, temps);        /* t4 = x1'*omega^2  */
    fpsqr_p256(t1, t3, temps);              /* t3 = theta^2  */
    fpsub_p256(t3, t5, t3);                 /* t3 = theta^2 - omega^3  */
    fpsub_p256(t3, t4, t3);                 /* t3 = theta^2 - omega^3 - x1'*omega^2  */
    fpsub_p256(t3, t4, P->X);               /* Xfinal = theta^2 - omega^3 - 2*x1'*omega^2  */
    fpsub_p256(P->X, t4, t3);               /* t3 = Xfinal-x1'*omega^2  */
    fpmul_p256(P->Y, t5, t2, temps);        /* t2 = y1'*omega^3  */
    fpmul_p256(t3, t1, t5, temps);          /* t5 = theta.(Xfinal-x1'*omega^2)  */
    fpsub_p256(t5, t2, P->Y);               /* Yfinal = theta.(Xfinal-x1'*omega^2) - y1'*omega^3  */

    /* cleanup */
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    fpzero_p256(t4);
    fpzero_p256(t5);
    fpzero_p256(t6);
    fpzero_p256(t7);
    ClearMemory(temps, sizeof(temps));
}

/* Special point addition R = P+Q with identical Z-coordinate for the precomputation
 * Weierstrass a=-3 curve
 * Inputs:  P = (X1,Y1,Z) in Jacobian coordinates with the same Z-coordinate
 *          Q = (X2,Y2,Z,Z^2,Z^3) in Chudnovsky coordinates with the same Z-coordinate
 *          Values (X1',Y1')
 * Outputs: R = (X3,Y3,Z3,Z3^2,Z3^2) in Chudnovsky coordinates
 *          new representation P  = (X1',Y1',Z1') = (X1.(X2-X1)^2, X1.(X2-X1)^3, Z.(X2-X1)) in Jacobian coordinates
 */
static void ecadd_precomp(ecpoint_jacobian_t* P, ecpoint_chudnovsky_t* Q, ecpoint_chudnovsky_t* R)
{
    digit256_t t1, t2, t3, t4;
    digit_t temps[P256_TEMPS];

    /* SECURITY NOTE: this function does not produce exceptions in the context of variable-base precomputation. */

    fpsub_p256(Q->X, P->X, t1);                 /* t1 = x2-x1  */
    fpmul_p256(P->Z, t1, R->Z, temps);          /* Zfinal = z.(x2-x1)  */
    fpcopy_p256(R->Z, P->Z);                    /* Z1' = z.(x2-x1)  */
    fpsqr_p256(t1, t2, temps);                  /* t2 = (x2-x1)^2  */
    fpsqr_p256(R->Z, R->Z2, temps);             /* Z2final = Zfinal^2  */
    fpmul_p256(t1, t2, t3, temps);              /* t3 = (x2-x1)^3  */
    fpmul_p256(P->X, t2, t4, temps);            /* t4 = X1' = x1.(x2-x1)^2  */
    fpcopy_p256(t4, P->X);                      /* X1'  */
    fpsub_p256(Q->Y, P->Y, t1);                 /* t1 = y2-y1  */
    fpsqr_p256(t1, R->X, temps);                /* X3 = (y2-y1)^2  */
    fpmul_p256(R->Z, R->Z2, R->Z3, temps);      /* Z3final = Zfinal^3  */
    fpsub_p256(R->X, t3, R->X);                 /* X3 = (y2-y1)^2 - (x2-x1)^3  */
    fpsub_p256(R->X, t4, R->X);                 /* X3 = (y2-y1)^2 - (x2-x1)^3 - x1.(x2-x1)^2  */
    fpsub_p256(R->X, t4, R->X);                 /* X3final = (y2-y1)^2 - (x2-x1)^3 - 2*x1.(x2-x1)^2  */
    fpsub_p256(t4, R->X, t2);                   /* t2 = x1.(x2-x1)^2-X3  */
    fpmul_p256(t1, t2, t4, temps);              /* t4 = (y2-y1)[x1.(x2-x1)^2-X3]  */
    fpmul_p256(P->Y, t3, t2, temps);            /* t2 = Y1' = y1*(x2-x1)^3  */
    fpcopy_p256(t2, P->Y);                      /* Y1'  */
    fpsub_p256(t4, t2, R->Y);                   /* Yfinal = (y2-y1)[x1.(x2-x1)^2-X3] - y1*(x2-x1)^3  */

    /* cleanup  */
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    fpzero_p256(t4);
    ClearMemory(temps, sizeof(temps));
}

/* Precomputation scheme using Jacobian coordinates
 * Weierstrass a=-3 curve
 * Input:   P = (x,y)
 * Outputs: T[0] = P, T[1] = 3*P, ... , T[npoints-1] = (2*npoints-1)*P in coordinates (X:Y:Z:Z^2:Z^3)
 */
static void ec_precomp(const ecpoint_t* P, ecpoint_chudnovsky_t* T, unsigned int npoints, ec_t* curve)
{
    ecpoint_jacobian_t P2;
    digit256_t t1, t2, t3;
    size_t i;
    digit_t temps[P256_TEMPS];

    QCC_UNUSED(curve);

    /* SECURITY NOTE: this function does not produce exceptions in the context of variable-base scalar multiplication and double-scalar multiplication. */

    /* Generating 2P = 2(x,y) = (X2,Y2,Z2) and P = (x,y) = (X1',Y1',Z1',Z1^2',Z1^3') = (x*y^2, y*y^3, y, y^2, y^3)  */
    fpzero_p256(t2); t2[0] = 1;                 /* t2 = 1  */
    fpsqr_p256(P->x, t1, temps);                /* t1 = x^2  */
    fpsub_p256(t1, t2, t1);                     /* t1 = x^2-1  */
    fpdiv2_p256(t1, t2, temps);                 /* t2 = (x^2-1)/2  */
    fpadd_p256(t1, t2, t1);                     /* t1 = alpha = 3(x^2-1)/2  */
    fpsqr_p256(P->y, T[0].Z2, temps);           /* Z1^2' = y^2  */
    fpmul_p256(T[0].Z2, P->x, T[0].X, temps);   /* X1' = beta = xy^2  */
    fpmul_p256(T[0].Z2, P->y, T[0].Z3, temps);  /* Z1^3' = y^3  */
    fpsqr_p256(t1, t2, temps);                  /* t2 = alpha^2  */
    fpsub_p256(t2, T[0].X, t2);                 /* t2 = alpha^2-beta  */
    fpsub_p256(t2, T[0].X, P2.X);               /* X2final = alpha^2-2beta  */
    fpcopy_p256(P->y, P2.Z);                    /* Z2final = y  */
    fpcopy_p256(P->y, T[0].Z);                  /* Z1' = y  */
    fpsqr_p256(T[0].Z2, T[0].Y, temps);         /* Y1' = y^4  */
    fpsub_p256(T[0].X, P2.X, t2);               /* t2 = beta-Xfinal  */
    fpmul_p256(t1, t2, t3, temps);              /* t3 = alpha.(beta-Xfinal)  */
    fpsub_p256(t3, T[0].Y, P2.Y);               /* Y2final = alpha.(beta-Xfinal)-y^4  */

    for (i = 1; i < npoints; i++) {
        /* T[i] = 2P'+T[i-1] = (2*i+1)P = (X_(2*i+1),Y_(2*i+1),Z_(2*i+1),Z_(2*i+1)^2,Z_(2*i+1)^3)
         * and new 2P' s.t. Z(2P')=Z_(2*i+1)
         */
        ecadd_precomp(&P2, &(T[i - 1]), &(T[i]));
    }

    /* cleanup */
    ecpoint_jacobian_zero(&P2);
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    ClearMemory(temps, sizeof(temps));
}

/* Constant-time table lookup to extract a Chudnovsky point (X:Y:Z:Z^2:Z^3) from the precomputed table
 * Weierstrass a=-3 curve
 * Operation: P = sign * table[(|digit|-1)/2], where sign=1 if digit>0 and sign=-1 if digit<0
 */
static void lut_chudnovsky(ecpoint_chudnovsky_t* table, ecpoint_chudnovsky_t* P, int digit, unsigned int npoints, ec_t* curve)
{
    unsigned int i, j;
    size_t nwords = NBITS_TO_NDIGITS(curve->pbits);
    digit_t sign, mask, pos;
    ecpoint_chudnovsky_t point, temp_point;

    sign = ((digit_t)digit >> (RADIX_BITS - 1)) - 1;                            /* if digit<0 then sign = 0x00...0 else sign = 0xFF...F */
    pos = ((sign & ((digit_t)digit ^ (digit_t)-digit)) ^ (digit_t)-digit) >> 1; /* position = (|digit|-1)/2  */
    fpcopy_p256(table[0].X, point.X);                                           /* point = table[0]  */
    fpcopy_p256(table[0].Y, point.Y);
    fpcopy_p256(table[0].Z, point.Z);
    fpcopy_p256(table[0].Z2, point.Z2);
    fpcopy_p256(table[0].Z3, point.Z3);

    for (i = 1; i < npoints; i++) {
        pos--;
        /* If match then mask = 0xFF...F else mask = 0x00...0 */
        mask = (digit_t)is_digit_nonzero_ct(pos) - 1;
        fpcopy_p256(table[i].X, temp_point.X);                            /* temp_point = table[i+1] */
        fpcopy_p256(table[i].Y, temp_point.Y);
        fpcopy_p256(table[i].Z, temp_point.Z);
        fpcopy_p256(table[i].Z2, temp_point.Z2);
        fpcopy_p256(table[i].Z3, temp_point.Z3);
        /* If mask = 0x00...0 then point = point, else if mask = 0xFF...F then point = temp_point */
        for (j = 0; j < nwords; j++) {
            point.X[j] = (mask & (point.X[j] ^ temp_point.X[j])) ^ point.X[j];
            point.Y[j] = (mask & (point.Y[j] ^ temp_point.Y[j])) ^ point.Y[j];
            point.Z[j] = (mask & (point.Z[j] ^ temp_point.Z[j])) ^ point.Z[j];
            point.Z2[j] = (mask & (point.Z2[j] ^ temp_point.Z2[j])) ^ point.Z2[j];
            point.Z3[j] = (mask & (point.Z3[j] ^ temp_point.Z3[j])) ^ point.Z3[j];
        }
    }

    fpcopy_p256(point.X, P->X);
    fpcopy_p256(point.Y, P->Y);
    fpcopy_p256(point.Z, P->Z);
    fpcopy_p256(point.Z2, P->Z2);
    fpcopy_p256(point.Z3, P->Z3);
    fpneg_p256(P->Y);                                                   /* point.Y: y coordinate  */
    fpcopy_p256(P->Y, temp_point.Y);                                    /* temp_point.Y: -y coordinate  */
    for (j = 0; j < nwords; j++) {                                      /* if sign = 0x00...0 then choose negative of the point  */
        point.Y[j] = (sign & (point.Y[j] ^ temp_point.Y[j])) ^ temp_point.Y[j];
    }
    fpcopy_p256(point.Y, P->Y);

    /* cleanup */
    ecpoint_chudnovsky_zero(&point);
    ecpoint_chudnovsky_zero(&temp_point);
}


/*
 * Evaluation for the complete addition
 * Determines the index for table lookup and the mask for element selections using complete_select
 */
static unsigned int lut_complete_eval(digit256_t val1, digit256_t val2, digit256_t val3, digit_t*mask)
{
    digit_t index_temp = 0, index = 3;
    digit_t eval1, eval2;

    eval1 = (digit_t)(fpiszero_p256(val1) - 1);                       /* if val1 = 0 then eval1 = 0, else eval1 = -1  */
    index = (eval1 & (index ^ index_temp)) ^ index_temp;              /* if val1 = 0 then index = 0  */

    index_temp = 2;
    eval2 = (digit_t)(fpiszero_p256(val3) - 1);                       /* if val3 = 0 then eval2 = 0, else eval2 = -1  */
    index = ((eval1 | eval2) & (index ^ index_temp)) ^ index_temp;    /* if (val1 = 0 & val3 = 0) then index = 2  */

    index_temp = 1;
    eval1 = (digit_t)(fpiszero_p256(val2) - 1);                       /* if val2 = 0 then eval1 = 0, else eval1 = -1  */
    index = (eval1 & (index ^ index_temp)) ^ index_temp;              /* if val2 = 0 then index = 1  */

    /* If index=3 then mask = 0xFF...F else mask = 0x00...0 */
    *mask = (digit_t)is_digit_nonzero_ct(index - 3) - 1;

    return (unsigned int)index;
}

/* Point extraction from LUT for the complete addition */
static void complete_lut(ecpoint_jacobian_t* table, unsigned int index, ecpoint_jacobian_t* P, unsigned int npoints, ec_t* curve)
{
    size_t i, j, nwords = NBITS_TO_NDIGITS(curve->pbits);
    digit_t pos, mask;
    ecpoint_jacobian_t point, temp_point;

    pos = (digit_t)index;                                                    // Load digit position
    ecpoint_jacobian_copy(&table[0], &point);                                       // point = table[0]

    for (i = 1; i < npoints; i++) {
        pos--;
        // If match then mask = 0xFF...F else sign = 0x00...0
        mask = (digit_t)is_digit_nonzero_ct(pos) - 1;
        ecpoint_jacobian_copy(&table[i], &temp_point);                              // temp_point = table[i+1]
        // If mask = 0x00...0 then point = point, else if mask = 0xFF...F then point = temp_point
        for (j = 0; j < nwords; j++) {
            point.X[j] = (mask & (point.X[j] ^ temp_point.X[j])) ^ point.X[j];
            point.Y[j] = (mask & (point.Y[j] ^ temp_point.Y[j])) ^ point.Y[j];
            point.Z[j] = (mask & (point.Z[j] ^ temp_point.Z[j])) ^ point.Z[j];
        }
    }
    ecpoint_jacobian_copy(&point, P);

    // cleanup
    ecpoint_jacobian_zero(&point);
    ecpoint_jacobian_zero(&temp_point);
}

/*
 * Field element selection for the complete addition
 * Operation: if mask = 0 then out = in1, else if mask = 0xff...ff then out = in2
 */
static void complete_select(digit256_t in1, digit256_t in2, digit256_t out, digit_t mask)
{
    size_t i = 0;
    size_t nwords = NBITS_TO_NDIGITS(sizeof(digit256_t) * 8);

    for (i = 0; i < nwords; i++) {
        out[i] = (mask & (in1[i] ^ in2[i])) ^ in1[i];
    }
}

/* Complete point addition: if P=-Q then P=0, else if P=0 then P=Q, else if P=Q then P=2P, else P=P+Q
 * Constant-time extraction over 5-LUT: table[0] = inf, table[1] = Q, table[2] = 2P, table[3] = P+Q, table[4] = P. First two entries and last one are assumed to be pre-loaded.
 * Weierstrass a=-3 curve
 * Inputs: P = (X1,Y1,Z1) in Jacobian coordinates
 *         Q = (X2,Y2,Z2) in Jacobian coordinates
 * Output: P = P+Q = (X1,Y1,Z1) + (X2,Y2,Z2) in Jacobian coordinates
 */
static void ecadd_jacobian_no_init(ecpoint_jacobian_t* Q, ecpoint_jacobian_t* P, ecpoint_jacobian_t* table, ec_t* curve)
{

    digit256_t t1, t2, t3, t4, t5, t6, t7, t8;
    unsigned int index = 0;
    digit_t mask = 0;
    digit_t mask1 = 0;
    digit_t temps[P256_TEMPS];

    /* SECURITY NOTE: this constant-time addition function is complete (i.e., it works for any possible inputs, including the cases P!=Q, P=Q, P=-Q and P=inf) on prime-order Weierstrass curves. */

    fpsqr_p256(P->Z, t2, temps);                        /* t2 = z1^2  */
    fpmul_p256(P->Z, t2, t3, temps);                    /* t3 = z1^3  */
    fpmul_p256(t2, Q->X, t1, temps);                    /* t1 = z1^2*x2  */
    fpmul_p256(t3, Q->Y, t4, temps);                    /* t4 = z1^3*y2  */
    fpsqr_p256(Q->Z, t3, temps);                        /* t3 = z2^2  */
    fpmul_p256(Q->Z, t3, t5, temps);                    /* t5 = z2^3  */
    fpmul_p256(t3, P->X, t7, temps);                    /* t7 = z2^2*x1  */
    fpmul_p256(t5, P->Y, t8, temps);                    /* t8 = z2^3*y1  */
    fpsub_p256(t1, t7, t1);                             /* t1 = beta2 = z1^2*x2-z2^2*x1   */
    fpsub_p256(t4, t8, t4);                             /* t4 = alpha2 = z1^3*y2-z2^3*y1  */
    index = lut_complete_eval(t1, P->Z, t4, &mask);     /* if t1=0 (P=-Q) then index=0, if Z1=0 (P inf) then index=1, if t4=0 (P=Q) then index=2, else index=3  */
    /* if index=3 then mask = 0xff...ff, else mask = 0 */
    mask1 = ~(-(int) fpiszero_p256(Q->Z));              /* if Z2=0 (Q inf) then mask1 = 0, else mask1 = 0xff...ff  */
    index = (mask1 & (index ^ 4)) ^ 4;                  /* if mask1 = 0 then index=4, else if mask1 = 0xff...ff then keep previous index    */
    fpadd_p256(P->X, t2, t3);                           /* t3 = x1+z1^2  */
    fpsub_p256(P->X, t2, t6);                           /* t6 = x1-z1^2  */
    complete_select(P->Y, t1, t2, mask);                /* If mask=0 (DBL) then t2=y1, else if mask=-1 (ADD) then t2=beta2   */
    fpsqr_p256(t2, t5, temps);                          /* t5 = y1^2 (DBL) or beta2^2 (ADD)  */
    complete_select(P->X, t7, t7, mask);                /* If mask=0 (DBL) then t7=x1, else if mask=-1 (ADD) then t7=z2^2*x1   */
    fpmul_p256(t5, t7, t1, temps);                      /* t1 = x1y1^2 = beta1 (DBL) or z2^2*x1*beta2^2 (ADD)  */
    fpmul_p256(P->Z, t2, table[2].Z, temps);            /* Z2Pfinal = z1y1  */
    fpmul_p256(Q->Z, table[2].Z, table[3].Z, temps);    /* ZPQfinal = z1*z2*beta2  */
    complete_select(t3, t2, t3, mask);                  /* If mask=0 (DBL) then t3=x1+z1^2, else if mask=-1 (ADD) then t3=beta2   */
    complete_select(t6, t5, t6, mask);                  /* If mask=0 (DBL) then t6=x1-z1^2, else if mask=-1 (ADD) then t6=beta2^2  */
    fpmul_p256(t3, t6, t2, temps);                      /* t2 = (x1+z1^2)(x1-z1^2) (DBL) or beta2^3 (ADD)  */
    fpdiv2_p256(t2, t3, temps);                         /* t3 = (x1+z1^2)(x1-z1^2)/2  */
    fpadd_p256(t2, t3, t3);                             /* t3 = alpha1 = 3(x1+z1^2)(x1-z1^2)/2  */
    complete_select(t3, t4, t3, mask);                  /* If mask=0 (DBL) then t3=alpha1, else if mask=-1 (ADD) then t3=alpha2  */
    fpsqr_p256(t3, t4, temps);                          /* t4 = alpha1^2 (DBL) or alpha2^2 (ADD)  */
    fpsub_p256(t4, t1, t4);                             /* t4 = alpha1^2-beta1 (DBL) or alpha2^2-z2^2*x1*beta2^2  */
    fpsub_p256(t4, t1, table[2].X);                     /* X2Pfinal = alpha1^2-2beta1 (DBL) or alpha2^2-2z2^2*x1*beta2^2 (ADD)  */
    fpsub_p256(table[2].X, t2, table[3].X);             /* XPQfinal = alpha^2-beta2^3-2z2^2*x1*beta2^2  */
    complete_select(table[2].X, table[3].X, t4, mask);  /* If mask=0 (DBL) then t4=X2Pfinal, else if mask=-1 (ADD) then t4=XPQfinal  */
    fpsub_p256(t1, t4, t1);                             /* t1 = beta1-X2Pfinal (DBL) or (ADD) z2^2*x1*beta2^2-XPQfinal  */
    fpmul_p256(t3, t1, t4, temps);                      /* t4 = alpha1.(beta1-X2Pfinal) or alpha2.(z2^2*x1*beta2^2-XPQfinal)  */
    complete_select(t5, t8, t1, mask);                  /* If mask=0 (DBL) then t1=y1^2, else if mask=-1 (ADD) then t1=z2^3*y1  */
    complete_select(t5, t2, t2, mask);                  /* If mask=0 (DBL) then t2=y1^2, else if mask=-1 (ADD) then t2=beta2^3  */
    fpmul_p256(t1, t2, t3, temps);                      /* t3 = y1^4 (DBL) or z2^3*y1*beta2^3 (ADD)  */
    fpsub_p256(t4, t3, table[2].Y);                     /* Y2Pfinal = alpha1.(beta1-X2Pfinal)-y1^4 (DBL) or alpha2.(z2^2*x1*beta2^2-XPQfinal)-z2^3*y1*beta2^3 (ADD)  */
    fpcopy_p256(table[2].Y, table[3].Y);                /* YPQfinal = alpha2.(z2^2*x1*beta2^2-XPQfinal)-z2^3*y1*beta2^3  */
    complete_lut(table, index, P, 5, curve);            /* P = table[index] (5 is the table size)  */

    /* cleanup */
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);
    fpzero_p256(t4);
    fpzero_p256(t5);
    fpzero_p256(t6);
    fpzero_p256(t7);
    fpzero_p256(t8);
    ClearMemory(temps, sizeof(temps));
}

/* Complete point addition: if P=-Q then P=0, else if P=0 then P=Q, else if P=Q then P=2P, else P=P+Q
 * Constant-time extraction over 5-LUT: table[0] = inf, table[1] = Q, table[2] = 2P, table[3] = P+Q, table[4] = P.
 * Weierstrass a=-3 curve
 * Inputs: P = (X1,Y1,Z1) in Jacobian coordinates
 *         Q = (X2,Y2,Z2) in Jacobian coordinates
 * Output: P = P+Q = (X1,Y1,Z1) + (X2,Y2,Z2) in Jacobian coordinates
 */
void ec_add_jacobian(ecpoint_jacobian_t* Q, ecpoint_jacobian_t* P, ec_t* curve)
{
    ecpoint_jacobian_t table[5] = { 0 };

    table[0].Y[0] = 1;                              /* Initialize table[0] with the point at infinity (0:1:0)  */
    ecpoint_jacobian_copy(Q, &table[1]);            /* Initialize table[1] with Q  */
    ecpoint_jacobian_copy(P, &table[4]);            /* Initialize table[4] with P  */
    ecadd_jacobian_no_init(Q, P, table, curve);

    /* cleanup */
    ecpoint_jacobian_zero(&table[0]);
    ecpoint_jacobian_zero(&table[1]);
    ecpoint_jacobian_zero(&table[2]);
    ecpoint_jacobian_zero(&table[3]);
    ecpoint_jacobian_zero(&table[4]);
}

/* Complete point addition for affine coordinates.
 * Computes P = P + Q
 */
void ec_add(ecpoint_t* P, const ecpoint_t* Q, ec_t* curve)
{
    ecpoint_jacobian_t Qj, Pj;

    ec_affine_tojacobian(P, &Pj);
    ec_affine_tojacobian(Q, &Qj);
    ec_add_jacobian(&Qj, &Pj, curve);           /* Pj = Pj + Qj */
    ec_toaffine(&Pj, P, curve);

    ecpoint_jacobian_zero(&Pj);
    ecpoint_jacobian_zero(&Qj);
}

/*  Is x < y? */
static inline unsigned char is_digit_lessthan_ct(digit_t x, digit_t y)
{
    return (unsigned char)((x ^ ((x ^ y) | ((x - y) ^ y))) >> (RADIX_BITS - 1));
}

/* Digit shift right */
#define SHIFTR(highIn, lowIn, shift, shiftOut)    \
    (shiftOut) = ((lowIn) >> (shift)) ^ ((highIn) << (RADIX_BITS - (shift)))

/* Computes the fixed window representation of scalar, where nonzero digits are in the set {+-1,+-3,...,+-(2^(w-1)-1)}  */
void fixed_window_recode(digit256_t scalar, unsigned int nbit, unsigned int w, int* digits)
{
    digit_t i, j, val, mask, t, cwords;
    digit_t temp, res, borrow;

    cwords = NBITS_TO_NDIGITS(nbit);            /* Number of computer words to represent scalar */
    t = (nbit + (w - 2)) / (w - 1);             /* Fixed length of the fixed window representation */
    mask = (1 << w) - 1;                        /* w-bit mask */
    val = (digit_t)(1 << (w - 1));              /* 2^(w-1)  */

    for (i = 0; i <= (t - 1); i++) {
        temp = (scalar[0] & mask) - val;        /* ki = (k mod 2^w) - 2^(w-1)  */
        *digits = (int)temp;
        digits++;

        res = scalar[0] - temp;                 /* k = (k - ki)  */
        borrow = ((temp >> (RADIX_BITS - 1)) - 1) & ((digit_t)is_digit_lessthan_ct(scalar[0], temp));
        scalar[0] = res;

        for (j = 1; j < cwords; j++) {
            res = scalar[j];
            scalar[j] = res - borrow;
            borrow = (digit_t)is_digit_lessthan_ct(res, borrow);
        }

        for (j = 0; j < cwords - 1; j++) {          /* k / 2^(w-1)  */
            SHIFTR(scalar[j + 1], scalar[j], (w - 1), scalar[j]);
        }
        scalar[cwords - 1] = scalar[cwords - 1] >> (w - 1);
    }
    *digits = (int)scalar[0];                     /* kt = k  (t+1 digits)  */

    // zero temporaries
    temp = res = borrow = 0;
    res = temp;  // prevent compiler removal
}

/* Number of digits in the representation of the scalar. W_VARBASE is the window size. */
#define DIGITS_TABLE_SIZE (((sizeof(digit256_t) * 8) + W_VARBASE - 2) / (W_VARBASE - 1) + 1)

/*
 * Variable-base scalar multiplication Q = k.P using fixed-window method
 * Weierstrass a=-3 curve
 */
QStatus ec_scalarmul(const ecpoint_t* P, digit256_t k, ecpoint_t* Q, ec_t* curve)
{
    unsigned int npoints = 1 << (W_VARBASE - 2);
    size_t num_digits = NBITS_TO_NDIGITS(curve->pbits);    /* Number of words to represent field elements and elements in modulo the group order */
    int digits[DIGITS_TABLE_SIZE] = { 0 };
    size_t t = (curve->rbits + (W_VARBASE - 2)) / (W_VARBASE - 1); /* Fixed length of the fixed window representation   */
    size_t i = 0;
    size_t j = 0;
    sdigit_t odd = 0;
    ecpoint_jacobian_t T;
    ecpoint_jacobian_t TT;
    ecpoint_chudnovsky_t table[1 << (W_VARBASE - 2)];
    ecpoint_chudnovsky_t R;
    digit256_t temp, t1, t2, t3;
    QStatus status = ER_FAIL;

    /* SECURITY NOTE: the crypto sensitive part of this function is protected against timing attacks and runs in constant-time on prime-order Weierstrass curves.
     *                Conditional if-statements evaluate public data only and the number of iterations for all loops is public.
     * DISCLAIMER:    the caller is responsible for guaranteeing that early termination produced after detecting errors during input validation
     *                (of scalar k or base point P) does not leak any secret information.
     */

    if (P == NULL || k == NULL || Q == NULL || curve == NULL) {
        return ER_INVALID_ADDRESS;
    }

    /*  Input validation: */
    /* Check if P is the point at infinity (0,0) */
    if (ec_is_infinity(P, curve) == B_TRUE) {
        return ER_INVALID_DATA;
    }
    /* Is scalar k in [1,r-1]?  */
    if ((fpiszero_p256(k) == true) || (validate_256(k, curve->order) == false)) {
        return ER_INVALID_DATA;
    }
    /* Are (x,y) in [0,p-1]? */
    if (fpvalidate_p256(P->x) == false || fpvalidate_p256(P->y) == false) {
        return ER_INVALID_DATA;
    }
    /* The question of if P lies on the curve should be checked before calling scalarmul */
    /* end input validation */

    ec_precomp(P, table, npoints, curve);               /* Precomputation of points T[0],...,T[npoints-1]  */

    odd = -((sdigit_t)k[0] & 1);
    fpsub_p256(curve->order, k, temp);                  /* Converting scalar to odd (r-k if even)  */
    for (j = 0; j < num_digits; j++) {                  /* If (even) then k = k_temp else k = k   */
        temp[j] = (odd & (k[j] ^ temp[j])) ^ temp[j];
    }

    fixed_window_recode(temp, (unsigned int)curve->rbits, W_VARBASE, digits);

    lut_chudnovsky(table, &R, digits[t], npoints, curve);
    fpcopy_p256(R.X, T.X);                              /* Initialize T = (X_T:Y_T:Z_T) with a point from the precomputed table */
    fpcopy_p256(R.Y, T.Y);
    fpcopy_p256(R.Z, T.Z);

    for (i = (t - 1); i >= 1; i--) {
        for (j = 0; j < (W_VARBASE - 2); j++) {
            ec_double_jacobian(&T);                     /* Double (X_T:Y_T:Z_T) = 2(X_T:Y_T:Z_T) */
        }
        lut_chudnovsky(table, &R, digits[i], npoints, curve);       /* Load R = (X_R:Y_R:Z_R:Z_R^2:Z_R^3) with a point from the precomputed table */
        ec_doubleadd(&R, &T, curve);                                /* Double-add (X_T:Y_T:Z_T) = 2(X_T:Y_T:Z_T) + (X_R:Y_R:Z_R:Z_R^2:Z_R^3) */
    }

    /* Perform last iteration  */
    for (j = 0; j < (W_VARBASE - 1); j++) {
        ec_double_jacobian(&T);                             /* Double (X_T:Y_T:Z_T) = 2(X_T:Y_T:Z_T)  */
    }
    lut_chudnovsky(table, &R, digits[0], npoints, curve);   /* Load R = (X_R:Y_R:Z_R:Z_R^2:Z_R^3) with a point from the precomputed table  */
    fpcopy_p256(R.X, TT.X);                                 /* TT = R = (X_R:Y_R:Z_R)  */
    fpcopy_p256(R.Y, TT.Y);
    fpcopy_p256(R.Z, TT.Z);
    ec_add_jacobian(&TT, &T, curve);                        /* Complete addition (X_T:Y_T:Z_T) = (X_T:Y_T:Z_T) + (X_R:Y_R:Z_R)  */

    fpcopy_p256(T.Y, temp);
    fpneg_p256(temp);                                       /* Correcting scalar (-Ty if even)  */

    for (j = 0; j < num_digits; j++) {                      /* If (even) then Ty = -Ty   */
        T.Y[j] = (odd & (T.Y[j] ^ temp[j])) ^ temp[j];
    }

    ec_toaffine(&T, Q, curve);                              /* Output Q = (x,y)  */
    status = ER_OK;

    ClearMemory(digits, DIGITS_TABLE_SIZE * sizeof(int));
    ecpoint_jacobian_zero(&T);
    ecpoint_jacobian_zero(&TT);
    ecpoint_chudnovsky_zero(&R);
    for (j = 0; j < (1 << (W_VARBASE - 2)); j++) {
        ecpoint_chudnovsky_zero(&table[j]);
    }
    fpzero_p256(temp);
    fpzero_p256(t1);
    fpzero_p256(t2);
    fpzero_p256(t3);

    return status;
}

}